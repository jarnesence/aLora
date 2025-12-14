#include <Arduino.h>
#include <cstring>
#include "config/AppBuildConfig.h"
#include "ui/Ui.h"
#include "util/Crypto.h"

static const char* kShortcuts[] = {"OK", "ACK", "OMW", "PING?", "HERE"};
static constexpr size_t kShortcutCount = sizeof(kShortcuts) / sizeof(kShortcuts[0]);

Ui::Ui(IDisplay* display, RotaryInput* input, MeshRadio* radio, ChatLog* log, PairingStore* pairs)
: _d(display), _in(input), _radio(radio), _log(log), _pairs(pairs) {}

void Ui::begin() {
  _lastDrawMs = 0;
  _screen = Screen::Menu;
  _menuIndex = 0;
  _radioField = 0;
  _pairingOption = 0;
  _pairingSelection = 0;
  _actionSelection = 0;
  _beaconActive = false;
  _listeningForPairs = false;
  resetCodeDigits();
  _focus = ComposeFocus::Contact;
}

void Ui::onIncoming(uint16_t src, const WireChatPacket& pkt) {
  uint16_t me = _radio ? _radio->localAddress() : 0;
  recordSeenPeer(src, _pairs && _pairs->hasKey(src));

  switch (pkt.kind) {
    case PacketKind::Ack:
      if (pkt.to != me) return;
      if (_log) _log->markDelivered(pkt.refMsgId);
      clearPending(pkt.refMsgId);
      noteDeliverySuccess(src);
      return;
    case PacketKind::Discovery:
      if (pkt.to != me && pkt.to != 0xFFFF) return;
      if (_radio) sendAck(src, pkt.refMsgId);
      return;
    case PacketKind::Presence:
      handlePresence(src, pkt);
      return;
    case PacketKind::PairRequest:
      handlePairRequest(src, pkt);
      return;
    case PacketKind::PairAccept:
      handlePairAccept(src, pkt);
      return;
    case PacketKind::SecureChat:
      handleSecureChat(src, pkt);
      return;
    case PacketKind::Chat:
    default:
      break;
  }

  if (pkt.to != me && pkt.to != 0xFFFF) return;

  if (_log) _log->add(src, false, pkt.msgId, pkt.text, (uint32_t)(millis()/1000));

  if (_radio) {
    sendAck(src, pkt.msgId);
  }
}

void Ui::handleInput() {
  if (!_in) return;
  _in->tick();

  bool click = _in->wasClicked();
  bool held = _in->wasHeld();
  int32_t delta = _in->readDelta();

  if (click) handleClick();
  if (held) handleHold();
  if (delta != 0) handleDelta(delta);
}

void Ui::handleClick() {
  switch (_screen) {
    case Screen::Menu:
      switch (_menuIndex % 4) {
        case 0: _screen = Screen::Contacts; return;
        case 1: _screen = Screen::Compose; _focus = ComposeFocus::Contact; return;
        case 2: _screen = Screen::Radio; return;
        case 3: _screen = Screen::Status; return;
        default: break;
      }
      return;
    case Screen::Contacts:
      selectContactTarget();
      if (_pairCandidate == UINT16_MAX) {
        _screen = Screen::PairMode;
      } else {
        _screen = Screen::Compose;
        _focus = ComposeFocus::Shortcut;
      }
      return;
    case Screen::Compose:
      handleComposeClick();
      return;
    case Screen::Radio:
      _radioField++;
      if (_radioField > 3) {
        _radioField = 0;
        _screen = Screen::Menu;
      }
      return;
    case Screen::PairMode:
      if ((_pairingOption % 2) == 0) {
        _beaconActive = true;
        _beaconStartMs = millis();
        _lastBeaconSendMs = 0;
        _screen = Screen::Broadcast;
      } else {
        _listeningForPairs = true;
        _pairingSelection = 0;
        for (size_t i = 0; i < kMaxBeacons; i++) {
          _beacons[i].active = false;
        }
        _screen = Screen::Listen;
      }
      return;
    case Screen::Broadcast:
      _beaconActive = false;
      _screen = Screen::Contacts;
      return;
    case Screen::Listen: {
      PairBeacon found{};
      if (beaconAt(_pairingSelection, found)) {
        _pairCandidate = found.addr;
        _pendingReqNonce = nextNonce();
        _pendingReqMsgId = _nextMsgId;
        sendPairRequest(found.addr);
        resetCodeDigits();
        _codeCursor = 0;
        _screen = Screen::CodeEntry;
      } else {
        _listeningForPairs = false;
        _screen = Screen::Contacts;
      }
      return;
    }
    case Screen::CodeEntry:
      if (_codeCursor < 3) {
        _codeCursor++;
      } else {
        confirmCodeEntry();
      }
      return;
    case Screen::ContactActions:
      if (_actionTarget != 0) {
        if (_actionSelection == 0) {
          _dst = _actionTarget;
          _screen = Screen::Compose;
          _focus = ComposeFocus::Shortcut;
        } else {
          for (size_t i = 0; i < kMaxSeenPeers; i++) {
            if (_seen[i].active && _seen[i].addr == _actionTarget) {
              _seen[i].active = false;
            }
          }
          _screen = Screen::Contacts;
        }
      } else {
        _screen = Screen::Contacts;
      }
      return;
    default:
      _screen = Screen::Menu;
      return;
  }
}

void Ui::handleHold() {
  switch (_screen) {
    case Screen::Contacts: {
      SeenPeer peer{};
      if (contactAt(_contactSelection, peer) && peer.addr != 0 && !peer.isNew) {
        _actionTarget = peer.addr;
        _actionSelection = 0;
        _screen = Screen::ContactActions;
      }
      break;
    }
    default:
      break;
  }
}

void Ui::handleDelta(int32_t delta) {
  switch (_screen) {
    case Screen::Menu: {
      int32_t next = (int32_t)_menuIndex + (delta > 0 ? 1 : -1);
      if (next < 0) next = 0;
      if (next > 3) next = 3;
      _menuIndex = (uint8_t)next;
      break;
    }
    case Screen::Contacts: {
      int32_t next = (int32_t)_contactScroll + (delta > 0 ? 1 : -1);
      int32_t max = (int32_t)contactListLength() - 1;
      if (next < 0) next = 0;
      if (next > max) next = max;
      _contactScroll = (uint8_t)next;
      _contactSelection = _contactScroll;
      break;
    }
    case Screen::Compose: {
      handleComposeDelta(delta);
      break;
    }
    case Screen::Radio:
      handleRadioDelta(delta);
      break;
    case Screen::PairMode: {
      int32_t next = (int32_t)_pairingOption + (delta > 0 ? 1 : -1);
      if (next < 0) next = 0;
      if (next > 1) next = 1;
      _pairingOption = (uint8_t)next;
      break;
    }
    case Screen::Listen: {
      size_t count = beaconCount();
      if (count == 0) break;
      int32_t next = (int32_t)_pairingSelection + (delta > 0 ? 1 : -1);
      if (next < 0) next = 0;
      if (next >= (int32_t)count) next = (int32_t)count - 1;
      _pairingSelection = (uint8_t)next;
      break;
    }
    case Screen::ContactActions: {
      int32_t next = (int32_t)_actionSelection + (delta > 0 ? 1 : -1);
      if (next < 0) next = 0;
      if (next > 1) next = 1;
      _actionSelection = (uint8_t)next;
      break;
    }
    case Screen::CodeEntry:
      handleCodeDelta(delta);
      break;
    case Screen::Status:
    case Screen::Settings:
    default:
      break;
  }
}

void Ui::handleComposeClick() {
  switch (_focus) {
    case ComposeFocus::Contact:
      selectContactTarget();
      _focus = ComposeFocus::Shortcut;
      break;
    case ComposeFocus::Shortcut:
      _focus = ComposeFocus::Send;
      break;
    case ComposeFocus::Send: {
      const char* phrase = kShortcuts[_shortcutIdx % kShortcutCount];
      std::strncpy(_draft, phrase, sizeof(_draft) - 1);
      _draft[sizeof(_draft) - 1] = '\0';
      sendDraft();
      _focus = ComposeFocus::Contact;
      _screen = Screen::Contacts;
      break;
    }
  }
}

void Ui::handleComposeDelta(int32_t delta) {
  switch (_focus) {
    case ComposeFocus::Contact: {
      size_t len = contactListLength();
      if (len <= 1) break;
      int32_t next = (int32_t)_contactSelection + (delta > 0 ? 1 : -1);
      int32_t max = (int32_t)len - 1;
      if (next < 0) next = 0;
      if (next > max) next = max;
      _contactSelection = (uint8_t)next;
      selectContactTarget();
      break;
    }
    case ComposeFocus::Shortcut: {
      if (delta == 0) break;
      int32_t next = (int32_t)_shortcutIdx + (delta > 0 ? 1 : -1);
      if (next < 0) next = (int32_t)kShortcutCount - 1;
      if (next >= (int32_t)kShortcutCount) next = 0;
      _shortcutIdx = (uint8_t)next;
      break;
    }
    case ComposeFocus::Send:
      // Keep steady; clicks handle send.
      break;
  }
}

void Ui::handleRadioDelta(int32_t delta) {
  int32_t sign = (delta > 0) ? 1 : -1;
  switch (_radioField % 4) {
    case 0: {
      int64_t nf = (int64_t)_uiFreqHz + (int64_t)sign * 50000;
      if (nf < 860000000) nf = 860000000;
      if (nf > 930000000) nf = 930000000;
      _uiFreqHz = (uint32_t)nf;
      break;
    }
    case 1: {
      int32_t nbw = (int32_t)_uiBwKHz + (int32_t)sign * 25;
      if (nbw < 62) nbw = 62;
      if (nbw > 500) nbw = 500;
      _uiBwKHz = (int8_t)nbw;
      break;
    }
    case 2: {
      int32_t nsf = (int32_t)_uiSf + sign;
      if (nsf < 6) nsf = 6;
      if (nsf > 12) nsf = 12;
      _uiSf = (uint8_t)nsf;
      break;
    }
    case 3: {
      int32_t ntx = (int32_t)_uiTxDbm + sign;
      if (ntx < 2) ntx = 2;
      if (ntx > 20) ntx = 20;
      _uiTxDbm = (int8_t)ntx;
      break;
    }
  }
}

void Ui::handleCodeDelta(int32_t delta) {
  int32_t sign = (delta > 0) ? 1 : -1;
  uint8_t idx = _codeCursor % 4;
  int32_t next = (int32_t)_codeDigits[idx] + sign;
  if (next < 0) next = 9;
  if (next > 9) next = 0;
  _codeDigits[idx] = (uint8_t)next;
  _pairCodeInput = codeValue();
}

void Ui::resetCodeDigits() {
  for (size_t i = 0; i < 4; i++) {
    _codeDigits[i] = 0;
  }
  _pairCodeInput = 0;
}

uint16_t Ui::codeValue() const {
  return (uint16_t)(_codeDigits[0] * 1000 + _codeDigits[1] * 100 + _codeDigits[2] * 10 + _codeDigits[3]);
}

uint16_t Ui::pairingCodeFor(uint16_t peer, uint32_t nonce) const {
  uint32_t mixed = ((uint32_t)peer * 37U) ^ 0x5A5A ^ nonce;
  return (uint16_t)((mixed % 9000U) + 1000U);
}

void Ui::confirmCodeEntry() {
  if (_pairCandidate == 0 || _pairCandidate == UINT16_MAX) {
    _screen = Screen::Contacts;
    return;
  }

  uint32_t acceptNonce = (uint32_t)_pairCodeInput;
  WireChatPacket req{};
  req.msgId = _pendingReqMsgId;
  req.nonce = _pendingReqNonce;
  sendPairAccept(_pairCandidate, req, acceptNonce);

  if (_pairs && _pendingReqMsgId != 0 && _pendingReqNonce != 0) {
    uint8_t key[PairingStore::kKeyLen] = {0};
    if (_pairs->deriveFromRequest(_pairCandidate, _pendingReqMsgId, _pendingReqNonce, acceptNonce, key)) {
      recordSeenPeer(_pairCandidate, true);
      _lastJoiner = _pairCandidate;
    }
  }
  _pendingReqMsgId = 0;
  _pendingReqNonce = 0;
  _pairCandidate = 0;
  _screen = Screen::Contacts;
}

char Ui::spinnerGlyph(uint32_t now) const {
  static const char frames[] = {'|', '/', '-', '\\'};
  size_t idx = ((now / 200) % 4);
  return frames[idx];
}

void Ui::selectContactTarget() {
  SeenPeer peer{};
  if (!contactAt(_contactSelection, peer)) return;

  if (peer.addr == 0) {
    _chatPeerFilter = 0;
    _pairCandidate = 0;
    return;
  }

  if (peer.isNew) {
    _pairCandidate = UINT16_MAX;
    _chatPeerFilter = 0;
    return;
  }

  _dst = peer.addr;
  _chatPeerFilter = peer.addr;
  _pairCandidate = peer.addr;
  recordSeenPeer(peer.addr, peer.paired);
}

void Ui::sendDraft() {
  if (!_radio) return;

  if (_pairs && _pairs->hasKey(_dst)) {
    sendSecureDraft();
    return;
  }

  if (!ensurePairedOrRequest(_dst)) {
    notePairing(_dst, "Pairing requested; waiting for accept.");
    return;
  }

  WireChatPacket pkt{};
  pkt.kind = PacketKind::Chat;
  pkt.msgId = _nextMsgId++;
  pkt.to = _dst;
  pkt.from = _radio->localAddress();
  pkt.ts = (uint32_t)(millis() / 1000);
  pkt.refMsgId = 0;
  pkt.nonce = 0;
  pkt.textLen = (uint16_t)::strnlen(_draft, sizeof(pkt.text));
  pkt.reserved = 0;
  std::strncpy(pkt.text, _draft, sizeof(pkt.text) - 1);
  pkt.text[sizeof(pkt.text) - 1] = '\0';

  bool sent = _radio->sendDm(_dst, pkt);
  uint32_t delay = computeRetryDelayMs(1);
  if (!sent && _radio) {
    uint32_t hold = _radio->msUntilAirtimeReset(millis());
    if (hold > delay) delay = hold;
  }
  recordPending(pkt, sent ? 1 : 0, delay);
  recordSeenPeer(_dst, _pairs && _pairs->hasKey(_dst));

  if (_log) {
    _log->add(_dst, true, pkt.msgId, pkt.text, pkt.ts);
  }
}

void Ui::sendSecureDraft() {
  if (!_radio || !_pairs) return;

  uint8_t key[PairingStore::kKeyLen] = {0};
  if (!_pairs->loadKey(_dst, key)) {
    sendPairRequest(_dst);
    notePairing(_dst, "Missing key; refreshed pairing request.");
    return;
  }

  WireChatPacket pkt{};
  pkt.kind = PacketKind::SecureChat;
  pkt.msgId = _nextMsgId++;
  pkt.to = _dst;
  pkt.from = _radio->localAddress();
  pkt.ts = (uint32_t)(millis() / 1000);
  pkt.refMsgId = 0;
  pkt.nonce = nextNonce();
  pkt.reserved = 0;

  size_t plainLen = ::strnlen(_draft, sizeof(pkt.text));
  if (plainLen > sizeof(pkt.text)) plainLen = sizeof(pkt.text);
  pkt.textLen = (uint16_t)plainLen;

  uint8_t cipher[sizeof(pkt.text)] = {0};
  bool ok = crypto::aes256_ctr_transform(
    key,
    _radio->localAddress(),
    _dst,
    pkt.nonce,
    pkt.msgId,
    reinterpret_cast<const uint8_t*>(_draft),
    plainLen,
    cipher
  );

  if (!ok) {
    notePairing(_dst, "Encryption failed; message not sent.");
    return;
  }

  memcpy(pkt.text, cipher, plainLen);
  if (plainLen < sizeof(pkt.text)) pkt.text[plainLen] = '\0';

  bool sent = _radio->sendDm(_dst, pkt);
  uint32_t delay = computeRetryDelayMs(1);
  if (!sent && _radio) {
    uint32_t hold = _radio->msUntilAirtimeReset(millis());
    if (hold > delay) delay = hold;
  }
  recordPending(pkt, sent ? 1 : 0, delay);
  recordSeenPeer(_dst, true);

  if (_log) {
    _log->add(_dst, true, pkt.msgId, _draft, pkt.ts);
  }
}

void Ui::recordPending(const WireChatPacket& pkt, uint8_t initialAttempts, uint32_t firstDelayMs) {
  for (size_t i = 0; i < kMaxPending; i++) {
    PendingSend& slot = _pending[i];
    if (slot.active) continue;
    slot.active = true;
    slot.dst = pkt.to;
    slot.attempts = initialAttempts;
    slot.discoverySent = false;
    slot.lastSendMs = (initialAttempts > 0) ? millis() : 0;
    slot.nextSendMs = millis() + firstDelayMs;
    slot.pkt = pkt;
    return;
  }
}

void Ui::clearPending(uint32_t msgId) {
  for (size_t i = 0; i < kMaxPending; i++) {
    PendingSend& slot = _pending[i];
    if (!slot.active) continue;
    if (slot.pkt.msgId != msgId) continue;
    slot.active = false;
    return;
  }
}

void Ui::maybeBroadcastPresence(uint32_t now) {
  if (!_radio) return;

  const uint32_t intervalMs = 30000;
  if (_lastPresenceMs != 0 && (now - _lastPresenceMs) < intervalMs) return;
  _lastPresenceMs = now;

  WireChatPacket pkt{};
  pkt.kind = PacketKind::Presence;
  pkt.msgId = _nextMsgId++;
  pkt.to = 0xFFFF;
  pkt.from = _radio->localAddress();
  pkt.ts = now / 1000;
  pkt.refMsgId = 0;
  pkt.nonce = nextNonce();
  std::strncpy(pkt.text, "aLora presence", sizeof(pkt.text) - 1);
  pkt.textLen = (uint16_t)::strnlen(pkt.text, sizeof(pkt.text));
  pkt.reserved = 0;

  _radio->sendDm(pkt.to, pkt);
}

void Ui::maybeSendPairBeacon(uint32_t now) {
  if (!_radio || !_beaconActive) return;

  const uint32_t intervalMs = 5000;
  if (_lastBeaconSendMs != 0 && (now - _lastBeaconSendMs) < intervalMs) return;
  _lastBeaconSendMs = now;

  WireChatPacket pkt{};
  pkt.kind = PacketKind::Presence;
  pkt.msgId = _nextMsgId++;
  pkt.to = 0xFFFF;
  pkt.from = _radio->localAddress();
  pkt.ts = now / 1000;
  pkt.refMsgId = 0;
  pkt.nonce = nextNonce();
  std::strncpy(pkt.text, "PAIR_BEACON", sizeof(pkt.text) - 1);
  pkt.textLen = (uint16_t)::strnlen(pkt.text, sizeof(pkt.text));
  pkt.reserved = 0;

  _radio->sendDm(pkt.to, pkt);
}

void Ui::handlePresence(uint16_t src, const WireChatPacket& pkt) {
  uint16_t me = _radio ? _radio->localAddress() : 0;
  if (pkt.to != 0xFFFF && pkt.to != me) return;
  if (std::strncmp(pkt.text, "PAIR_BEACON", sizeof(pkt.text)) == 0) {
    rememberBeacon(src, millis());
    recordSeenPeer(src, false);
    return;
  }

  notePairing(src, "Presence seen.");
}

void Ui::handlePairRequest(uint16_t src, const WireChatPacket& pkt) {
  uint16_t me = _radio ? _radio->localAddress() : 0;
  if (pkt.to != me && pkt.to != 0xFFFF) return;
  if (!_pairs) return;

  _pairCandidate = src;
  _pendingReqNonce = pkt.nonce;
  _pendingReqMsgId = pkt.msgId;
  _lastJoiner = 0;
  _beaconActive = true;
  _screen = Screen::Broadcast;
  notePairing(src, "Pair request received; show PIN");
}

void Ui::handlePairAccept(uint16_t src, const WireChatPacket& pkt) {
  uint16_t me = _radio ? _radio->localAddress() : 0;
  if (pkt.to != me && pkt.to != 0xFFFF) return;
  if (!_pairs) return;

  uint8_t key[PairingStore::kKeyLen] = {0};
  bool paired = false;

  if (_pendingReqMsgId != 0 && _pendingReqNonce != 0 && src == _pairCandidate && pkt.refMsgId == _pendingReqMsgId) {
    if (_pairs->deriveFromRequest(src, _pendingReqMsgId, _pendingReqNonce, pkt.nonce, key)) {
      paired = true;
    }
  }

  if (!paired && _pairs->resolvePendingRequest(src, pkt.refMsgId, pkt.nonce, key)) {
    paired = true;
  }

  if (paired) {
    notePairing(src, "Pairing accepted.");
    recordSeenPeer(src, true);
    _lastJoiner = src;
    _pendingReqMsgId = 0;
    _pendingReqNonce = 0;
    _pairCandidate = 0;
    _beaconActive = false;
    _listeningForPairs = false;
    _screen = Screen::Contacts;
  }
}

void Ui::handleSecureChat(uint16_t src, const WireChatPacket& pkt) {
  uint16_t me = _radio ? _radio->localAddress() : 0;
  if (pkt.to != me) return;
  if (!_pairs) return;

  if (!_pairs->hasKey(src)) {
    sendPairRequest(src);
    notePairing(src, "Secure packet seen without key; re-requesting pair.");
    return;
  }

  if (!_pairs->checkReplayAndUpdate(src, pkt.msgId)) return;

  char plain[sizeof(pkt.text) + 1] = {0};
  if (!decryptSecureText(src, pkt, plain, sizeof(plain))) return;

  if (_log) _log->add(src, false, pkt.msgId, plain, (uint32_t)(millis()/1000));
  if (_radio) sendAck(src, pkt.msgId);
}

bool Ui::decryptSecureText(uint16_t src, const WireChatPacket& pkt, char* outText, size_t outLen) {
  if (!_pairs || !_radio) return false;
  uint8_t key[PairingStore::kKeyLen] = {0};
  if (!_pairs->loadKey(src, key)) return false;

  size_t len = pkt.textLen;
  if (len > sizeof(pkt.text)) len = sizeof(pkt.text);
  if (len >= outLen) len = outLen - 1;

  bool ok = crypto::aes256_ctr_transform(
    key,
    _radio->localAddress(),
    src,
    pkt.nonce,
    pkt.msgId,
    reinterpret_cast<const uint8_t*>(pkt.text),
    len,
    reinterpret_cast<uint8_t*>(outText)
  );

  if (!ok) return false;
  outText[len] = '\0';
  return true;
}

void Ui::updateReliability() {
  if (!_radio) return;

  const uint32_t now = millis();
  const uint8_t maxUnicastAttempts = 3;
  const uint8_t maxTotalAttempts = 5;
  const uint32_t discoveryCooldownMs = 5000;
  const uint32_t airtimeDeferralFloorMs = 1200;

  for (size_t i = 0; i < kMaxPending; i++) {
    PendingSend& slot = _pending[i];
    if (!slot.active) continue;

    bool recentlySent = (slot.lastSendMs != 0) && ((now - slot.lastSendMs) < 2000);
    if (!slot.discoverySent && slot.attempts > 0 && !recentlySent && routeIsStale(slot.dst, now)) {
      if (escalateDiscovery(slot, now)) {
        slot.nextSendMs = now + discoveryCooldownMs;
        continue;
      }
    }

    if (slot.attempts >= maxUnicastAttempts && !slot.discoverySent) {
      if (now >= slot.nextSendMs) {
        if (escalateDiscovery(slot, now)) {
          slot.nextSendMs = now + discoveryCooldownMs;
        }
      }
      continue;
    }

    if (slot.attempts >= maxTotalAttempts) {
      if (now >= slot.nextSendMs) {
        slot.active = false;
        if (_log) _log->markFailed(slot.pkt.msgId);
      }
      continue;
    }

    if (now < slot.nextSendMs) continue;

    if (!_radio->sendDm(slot.dst, slot.pkt)) {
      uint32_t hold = _radio->msUntilAirtimeReset(now);
      if (hold < airtimeDeferralFloorMs) hold = airtimeDeferralFloorMs;
      slot.nextSendMs = now + hold;
      continue;
    }

    slot.attempts++;
    slot.lastSendMs = now;
    slot.nextSendMs = now + computeRetryDelayMs(slot.attempts);
  }
}

bool Ui::escalateDiscovery(PendingSend& slot, uint32_t now) {
  if (!_radio) return false;
  bool sent = _radio->sendDiscovery(slot.dst, slot.pkt.msgId);
  if (sent) {
    RouteHealth* r = routeFor(slot.dst);
    if (r) r->lastDiscoveryMs = now;
    slot.discoverySent = true;
    slot.lastSendMs = now;
  } else {
    slot.nextSendMs = now + computeRetryDelayMs(slot.attempts + 1);
  }
  return sent;
}

uint32_t Ui::computeRetryDelayMs(uint8_t attempt) const {
  const uint32_t baseDelayMs = 2500;
  const uint32_t jitterWindowMs = 600;
  uint32_t jitterSeed = (uint32_t)millis();
  uint32_t jitter = jitterSeed % jitterWindowMs;
  return (baseDelayMs * (uint32_t)attempt) + jitter;
}

void Ui::sendPairRequest(uint16_t dst) {
  if (!_radio || !_pairs) return;

  WireChatPacket pkt{};
  pkt.kind = PacketKind::PairRequest;
  pkt.msgId = _nextMsgId++;
  pkt.to = dst;
  pkt.from = _radio->localAddress();
  pkt.ts = (uint32_t)(millis() / 1000);
  pkt.refMsgId = 0;
  if (_pendingReqNonce == 0) _pendingReqNonce = nextNonce();
  pkt.nonce = _pendingReqNonce;
  pkt.textLen = 0;
  pkt.reserved = 0;
  std::strncpy(pkt.text, "PAIR_REQ", sizeof(pkt.text) - 1);

  _radio->sendDm(dst, pkt);
  _pairs->recordOutgoingRequest(dst, pkt.msgId, pkt.nonce);
  _pendingReqMsgId = pkt.msgId;
  notePairing(dst, "Sent pairing request.");
  recordSeenPeer(dst, false);
}

void Ui::sendPairAccept(uint16_t dst, const WireChatPacket& req, uint32_t acceptNonce) {
  if (!_radio || !_pairs) return;
  WireChatPacket pkt{};
  pkt.kind = PacketKind::PairAccept;
  pkt.msgId = _nextMsgId++;
  pkt.to = dst;
  pkt.from = _radio->localAddress();
  pkt.ts = (uint32_t)(millis() / 1000);
  pkt.refMsgId = req.msgId;
  pkt.nonce = acceptNonce;
  pkt.textLen = 0;
  pkt.reserved = 0;
  std::strncpy(pkt.text, "PAIR_ACCEPT", sizeof(pkt.text) - 1);

  _radio->sendDm(dst, pkt);
}

void Ui::notePairing(uint16_t peer, const char* msg) {
  if (!_log) return;
  _log->add(peer, false, 0, msg, (uint32_t)(millis()/1000));
}

bool Ui::ensurePairedOrRequest(uint16_t dst) {
  if (!_pairs) return true;
  if (_pairs->hasKey(dst)) return true;
  sendPairRequest(dst);
  return false;
}

uint32_t Ui::nextNonce() const {
  uint32_t seed = (uint32_t)millis();
  seed ^= ((uint32_t)_nextMsgId << 1);
  if (_radio) seed ^= ((uint32_t)_radio->localAddress() << 16);
  return seed ^ 0xA5A5BEEF;
}

void Ui::recordSeenPeer(uint16_t addr, bool paired) {
  if (addr == 0) return;

  uint32_t now = (uint32_t)(millis() / 1000);
  size_t slot = kMaxSeenPeers;
  for (size_t i = 0; i < kMaxSeenPeers; i++) {
    if (_seen[i].active && _seen[i].addr == addr) { slot = i; break; }
  }

  if (slot == kMaxSeenPeers) {
    for (size_t i = 0; i < kMaxSeenPeers; i++) {
      if (!_seen[i].active) { slot = i; break; }
    }
  }

  if (slot == kMaxSeenPeers) {
    uint32_t oldest = UINT32_MAX;
    size_t oldestIdx = 0;
    for (size_t i = 0; i < kMaxSeenPeers; i++) {
      if (_seen[i].lastSeenSec <= oldest) {
        oldest = _seen[i].lastSeenSec;
        oldestIdx = i;
      }
    }
    slot = oldestIdx;
  }

  SeenPeer& s = _seen[slot];
  s.active = true;
  s.addr = addr;
  s.lastSeenSec = now;
  if (paired) s.paired = true;
}

size_t Ui::contactCount() const {
  size_t n = 0;
  for (size_t i = 0; i < kMaxSeenPeers; i++) {
    if (_seen[i].active) n++;
  }
  return n;
}

size_t Ui::contactListLength() const {
  // include "All" row and "New" action row
  return contactCount() + 2;
}

bool Ui::contactAt(size_t idx, SeenPeer& out) const {
  if (idx == 0) {
    out.active = true;
    out.addr = 0;
    out.paired = false;
    out.lastSeenSec = 0;
    out.isNew = false;
    return true;
  }

  if (idx == contactListLength() - 1) {
    out.active = true;
    out.addr = 0;
    out.paired = false;
    out.lastSeenSec = 0;
    out.isNew = true;
    return true;
  }

  size_t rank = idx - 1;
  bool used[kMaxSeenPeers] = {false};

  for (size_t r = 0; r <= rank; r++) {
    size_t bestSlot = kMaxSeenPeers;
    uint32_t bestSeen = 0;
    for (size_t i = 0; i < kMaxSeenPeers; i++) {
      if (!_seen[i].active || used[i]) continue;
      if (bestSlot == kMaxSeenPeers || _seen[i].lastSeenSec > bestSeen) {
        bestSlot = i;
        bestSeen = _seen[i].lastSeenSec;
      }
    }

    if (bestSlot == kMaxSeenPeers) return false;
    if (r == rank) {
      out = _seen[bestSlot];
      return true;
    }
    used[bestSlot] = true;
  }

  return false;
}

void Ui::rememberBeacon(uint16_t addr, uint32_t now) {
  for (size_t i = 0; i < kMaxBeacons; i++) {
    if (_beacons[i].active && _beacons[i].addr == addr) {
      _beacons[i].lastSeenMs = now;
      return;
    }
  }

  for (size_t i = 0; i < kMaxBeacons; i++) {
    if (!_beacons[i].active) {
      _beacons[i].active = true;
      _beacons[i].addr = addr;
      _beacons[i].lastSeenMs = now;
      return;
    }
  }

  size_t oldest = 0;
  for (size_t i = 1; i < kMaxBeacons; i++) {
    if (_beacons[i].lastSeenMs < _beacons[oldest].lastSeenMs) oldest = i;
  }
  _beacons[oldest].active = true;
  _beacons[oldest].addr = addr;
  _beacons[oldest].lastSeenMs = now;
}

size_t Ui::beaconCount() const {
  size_t n = 0;
  for (size_t i = 0; i < kMaxBeacons; i++) {
    if (_beacons[i].active) n++;
  }
  return n;
}

bool Ui::beaconAt(size_t idx, PairBeacon& out) const {
  bool used[kMaxBeacons] = {false};
  for (size_t r = 0; r <= idx; r++) {
    size_t best = kMaxBeacons;
    uint32_t newest = 0;
    for (size_t i = 0; i < kMaxBeacons; i++) {
      if (!_beacons[i].active || used[i]) continue;
      if (best == kMaxBeacons || _beacons[i].lastSeenMs > newest) {
        best = i;
        newest = _beacons[i].lastSeenMs;
      }
    }
    if (best == kMaxBeacons) return false;
    if (r == idx) {
      out = _beacons[best];
      return true;
    }
    used[best] = true;
  }
  return false;
}

const ChatMsg* Ui::latestMessage(uint16_t filter) const {
  if (!_log) return nullptr;
  int total = (int)_log->size();
  for (int i = total - 1; i >= 0; --i) {
    const ChatMsg& msg = _log->at((size_t)i);
    if (filter == 0 || msg.src == filter) return &msg;
  }
  return nullptr;
}

Ui::RouteHealth* Ui::routeFor(uint16_t dst) {
  size_t freeIdx = kMaxRoutes;
  size_t oldestIdx = 0;
  uint32_t oldestTs = 0xFFFFFFFFUL;
  bool haveOld = false;

  for (size_t i = 0; i < kMaxRoutes; i++) {
    RouteHealth& r = _routes[i];
    if (r.active && r.dst == dst) return &r;
    if (!r.active && freeIdx == kMaxRoutes) freeIdx = i;

    uint32_t freshness = r.lastAckMs ? r.lastAckMs : r.lastDiscoveryMs;
    if (!r.active) freshness = 0;
    if (!haveOld || freshness < oldestTs) {
      haveOld = true;
      oldestIdx = i;
      oldestTs = freshness;
    }
  }

  if (freeIdx != kMaxRoutes) return &_routes[freeIdx];
  return &_routes[oldestIdx];
}

const Ui::RouteHealth* Ui::routeFor(uint16_t dst) const {
  for (size_t i = 0; i < kMaxRoutes; i++) {
    const RouteHealth& r = _routes[i];
    if (r.active && r.dst == dst) return &r;
  }
  return nullptr;
}

void Ui::noteDeliverySuccess(uint16_t dst) {
  RouteHealth* r = routeFor(dst);
  if (!r) return;
  r->active = true;
  r->dst = dst;
  if (r->successStreak < 250) r->successStreak++;
  r->lastAckMs = millis();
}

bool Ui::routeIsStale(uint16_t dst, uint32_t now) const {
  const RouteHealth* r = routeFor(dst);
  const uint32_t freshnessMs = 45000;
  if (!r) return true;
  uint32_t anchor = r->lastAckMs ? r->lastAckMs : r->lastDiscoveryMs;
  if (anchor == 0) return true;
  return (now - anchor) > freshnessMs;
}

void Ui::sendAck(uint16_t dst, uint32_t refMsgId) {
  WireChatPacket ack{};
  ack.kind = PacketKind::Ack;
  ack.msgId = _nextMsgId++;
  ack.to = dst;
  ack.from = _radio->localAddress();
  ack.ts = (uint32_t)(millis() / 1000);
  ack.refMsgId = refMsgId;
  ack.nonce = 0;
  ack.textLen = 0;
  ack.reserved = 0;
  ack.text[0] = '\0';
  _radio->sendDm(dst, ack);
}

void Ui::tick() {
  handleInput();
  updateReliability();

  // Draw at ~10 FPS
  uint32_t now = millis();
  maybeBroadcastPresence(now);
  maybeSendPairBeacon(now);
  if (now - _lastDrawMs < 100) return;
  _lastDrawMs = now;

  if (!_d) return;
  _d->clear();

  switch (_screen) {
    case Screen::Menu: drawMenu(); break;
    case Screen::Contacts: drawContacts(); break;
    case Screen::Compose: drawCompose(); break;
    case Screen::Radio: drawRadio(); break;
    case Screen::PairMode: drawPairMode(); break;
    case Screen::Broadcast: drawBroadcast(); break;
    case Screen::Listen: drawListen(); break;
    case Screen::CodeEntry: drawCodeEntry(); break;
    case Screen::ContactActions: drawContactActions(); break;
    case Screen::Status: drawStatus(); break;
    case Screen::Settings: drawSettings(); break;
  }

  _d->send();
}

void Ui::drawMenu() {
  int w = _d->width();
  _d->setFontUi();
  _d->drawStr(4, 12, "MENU");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();
  const char* items[] = {"Contacts", "Canned chat", "Radio", "Status"};
  int lineH = 14;
  int yBase = 28;
  for (size_t i = 0; i < 4; i++) {
    int y = yBase + (int)i * lineH;
    if (i == (_menuIndex % 4)) {
      _d->drawFrame(0, y - 10, w, lineH);
    }
    _d->drawStr(6, y, items[i]);
  }
  _d->drawStr(6, yBase + lineH * 4, "Rotary: left/right  | Click: enter");
}

void Ui::drawContacts() {
  int w = _d->width();
  _d->setFontUi();
  _d->drawStr(2, 12, "CONTACTS");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();
  size_t len = contactListLength();
  if (len <= 1) {
    char wait[24];
    snprintf(wait, sizeof(wait), "Listening %c", spinnerGlyph(millis()));
    _d->drawStr(2, 32, wait);
    _d->drawStr(2, 44, "No peers yet");
    return;
  }

  _d->drawStr(2, 26, "Rotary: scroll  Click: open  Hold: actions");

  uint32_t now = (uint32_t)(millis() / 1000);
  int lineH = 12;
  int yBase = 44;

  for (size_t row = 0; row < 6; row++) {
    size_t idx = (size_t)_contactScroll + row;
    if (idx >= len) break;

    SeenPeer peer{};
    if (!contactAt(idx, peer)) continue;

    char line[32];
    if (peer.addr == 0) {
      std::snprintf(line, sizeof(line), "[ALL CHATS]");
    } else if (peer.isNew) {
      std::snprintf(line, sizeof(line), "[NEW]");
    } else {
      uint32_t age = (peer.lastSeenSec <= now) ? (now - peer.lastSeenSec) : 0;
      std::snprintf(line, sizeof(line), "%c%u (%lus)", peer.paired ? 'P' : 'S', (unsigned)peer.addr, (unsigned long)age);
    }

    int y = yBase + (int)(row * lineH);
    if (idx == _contactSelection) {
      _d->drawFrame(0, y - 2, w, lineH);
    }
    _d->drawStr(2, y, line);
  }
}

void Ui::drawRadio() {
  int w = _d->width();
  _d->setFontUi();
  _d->drawStr(2, 12, "RADIO");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();
  _d->drawStr(2, 26, "Rotate: adjust  Click: next");

  char line[32];
  const char* labels[] = {"Freq", "BW", "SF", "TX"};
  uint32_t values[] = {_uiFreqHz, (uint32_t)_uiBwKHz, (uint32_t)_uiSf, (uint32_t)_uiTxDbm};
  const char* suffix[] = {"Hz", "kHz", "", " dBm"};

  int lineH = 14;
  int yBase = 42;
  for (size_t i = 0; i < 4; i++) {
    int y = yBase + (int)i * lineH;
    snprintf(line, sizeof(line), "%s:%lu%s", labels[i], (unsigned long)values[i], suffix[i]);
    if (i == (_radioField % 4)) {
      _d->drawFrame(0, y - 10, w, lineH);
    }
    _d->drawStr(4, y, line);
  }

  _d->drawStr(4, yBase + lineH * 4, "Applies on next profile load");
}

void Ui::drawPairMode() {
  int w = _d->width();
  _d->setFontUi();
  _d->drawStr(2, 12, "NEW LINK");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();
  _d->drawStr(2, 26, "Pick how to connect:");

  const char* options[] = {"Broadcast", "Listen"};
  int lineH = 14;
  int yBase = 42;
  for (size_t i = 0; i < 2; i++) {
    int y = yBase + (int)i * lineH;
    if (i == (_pairingOption % 2)) {
      _d->drawFrame(0, y - 10, w, lineH);
    }
    _d->drawStr(4, y, options[i]);
  }

  _d->drawStr(2, yBase + lineH * 2, "Click to confirm");
}

void Ui::drawBroadcast() {
  int w = _d->width();
  _d->setFontUi();
  _d->drawStr(2, 12, "BROADCAST");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();
  char line[32];
  snprintf(line, sizeof(line), "Sending %c", spinnerGlyph(millis()));
  _d->drawStr(2, 32, line);
  _d->drawStr(2, 44, "Every 5s until cancel");

  if (_pairCandidate != 0 && _pendingReqNonce != 0) {
    uint16_t pin = pairingCodeFor(_pairCandidate, _pendingReqNonce);
    char pinLine[24];
    snprintf(pinLine, sizeof(pinLine), "PIN:%04u", (unsigned)pin);
    _d->drawStr(2, 60, "Incoming request!");
    _d->drawStr(2, 74, pinLine);
  }

  if (_lastJoiner != 0) {
    char joined[32];
    snprintf(joined, sizeof(joined), "Paired: %u", (unsigned)_lastJoiner);
    _d->drawStr(2, 88, joined);
  }

  _d->drawStr(2, 102, "Click to stop");
}

void Ui::drawListen() {
  int w = _d->width();
  _d->setFontUi();
  _d->drawStr(2, 12, "LISTEN");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();
  _d->drawStr(2, 26, "Watching for broadcasts");

  size_t count = beaconCount();
  if (count == 0) {
    char wait[28];
    snprintf(wait, sizeof(wait), "Waiting %c", spinnerGlyph(millis()));
    _d->drawStr(2, 42, wait);
    _d->drawStr(2, 56, "Click any time to cancel");
    return;
  }

  int lineH = 14;
  int yBase = 46;
  for (size_t row = 0; row < 4; row++) {
    size_t idx = (size_t)_pairingSelection + row;
    if (idx >= count) break;
    PairBeacon b{};
    if (!beaconAt(idx, b)) continue;

    char line[28];
    snprintf(line, sizeof(line), "Peer:%u", (unsigned)b.addr);
    int y = yBase + (int)row * lineH;
    if (idx == _pairingSelection) {
      _d->drawFrame(0, y - 2, w, lineH);
    }
    _d->drawStr(4, y, line);
  }

  _d->drawStr(2, yBase + lineH * 4, "Click to request PIN");
}

void Ui::drawCodeEntry() {
  int w = _d->width();
  _d->setFontUi();
  _d->drawStr(2, 12, "PAIR CODE");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();
  char target[28];
  snprintf(target, sizeof(target), "Device: %u", (unsigned)_pairCandidate);
  _d->drawStr(2, 30, target);
  _d->drawStr(2, 42, "Rotate digit, click next");

  char codeLine[8] = {0};
  for (size_t i = 0; i < 4; i++) {
    codeLine[i] = '0' + _codeDigits[i];
  }
  _d->drawStr(2, 58, codeLine);
  int cursorX = 2 + (int)_codeCursor * 8;
  _d->drawVLine(cursorX, 50, 20);

  _d->drawStr(2, 74, "Code shown on other device");
}

void Ui::drawContactActions() {
  int w = _d->width();
  _d->setFontUi();
  _d->drawStr(2, 12, "ACTIONS");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();
  if (_actionTarget == 0) {
    _d->drawStr(2, 32, "No contact selected");
    return;
  }

  char target[28];
  snprintf(target, sizeof(target), "Target:%u", (unsigned)_actionTarget);
  _d->drawStr(2, 30, target);
  _d->drawStr(2, 42, "Rotate to choose, click");

  const char* options[] = {"Message", "Forget"};
  int lineH = 14;
  int yBase = 58;
  for (size_t i = 0; i < 2; i++) {
    int y = yBase + (int)i * lineH;
    if ((int)i == _actionSelection) {
      _d->drawFrame(0, y - 10, w, lineH);
    }
    _d->drawStr(4, y, options[i]);
  }
}

void Ui::drawCompose() {
  int w = _d->width();

  _d->setFontUi();
  _d->drawStr(2, 12, "CANNED MSG");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();
  SeenPeer sel{};
  contactAt(_contactSelection, sel);
  char contactLine[28];
  if (sel.addr == 0) snprintf(contactLine, sizeof(contactLine), "To:All chats");
  else snprintf(contactLine, sizeof(contactLine), "To:%u%s", (unsigned)sel.addr, sel.paired ? "*" : "");

  if (_focus == ComposeFocus::Contact) {
    _d->drawFrame(0, 18, w, 14);
  }
  _d->drawStr(2, 28, contactLine);

  _d->drawStr(2, 42, "Canned options:");
  int lineH = 12;
  int yBase = 56;
  for (size_t i = 0; i < kShortcutCount; i++) {
    int y = yBase + (int)i * lineH;
    if (_focus == ComposeFocus::Shortcut && i == (_shortcutIdx % kShortcutCount)) {
      _d->drawFrame(0, y - 10, w, lineH);
    }
    _d->drawStr(4, y, kShortcuts[i]);
  }

  if (_focus == ComposeFocus::Send) {
    _d->drawFrame(0, yBase + lineH * (int)kShortcutCount + 2, w, 14);
  }
  _d->drawStr(4, yBase + lineH * (int)kShortcutCount + 12, "Click to send selection");

  const ChatMsg* last = latestMessage(_chatPeerFilter);
  if (last) {
    char recent[24];
    const char* state = "";
    if (last->outgoing) {
      if (last->failed) state = "!";
      else state = last->delivered ? "✓" : "…";
    }
    snprintf(recent, sizeof(recent), "%c%u%s", last->outgoing ? '>' : '<', (unsigned)last->src, state);
    _d->drawStr(2, 108, recent);

    char snippet[24];
    std::strncpy(snippet, last->text, sizeof(snippet)-1);
    snippet[sizeof(snippet)-1] = '\0';
    _d->drawStr(2, 120, snippet);
  } else {
    _d->drawStr(2, 108, "No messages yet.");
  }
}

void Ui::drawStatus() {
  int w = _d->width();

  _d->setFontUi();
  _d->drawStr(2, 12, "STATS");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();

  char line[26];

  snprintf(line, sizeof(line), "RX:%lu", (unsigned long)_radio->rxCount());
  _d->drawStr(2, 30, line);

  snprintf(line, sizeof(line), "TX:%lu", (unsigned long)_radio->txCount());
  _d->drawStr(2, 42, line);

  snprintf(line, sizeof(line), "Air:%lums", (unsigned long)_radio->txAirtimeMs());
  _d->drawStr(2, 54, line);

  snprintf(line, sizeof(line), "Pend:%u", (unsigned)pendingCount());
  _d->drawStr(2, 66, line);

  snprintf(line, sizeof(line), "Uptime:%lus", (unsigned long)(millis()/1000));
  _d->drawStr(2, 78, line);

  const ChatMsg* last = latestMessage(0);
  if (last) {
    char recent[24];
    const char* state = "";
    if (last->outgoing) {
      if (last->failed) state = "!";
      else state = last->delivered ? "✓" : "…";
    }
    snprintf(recent, sizeof(recent), "Last %c%u%s", last->outgoing ? '>' : '<', (unsigned)last->src, state);
    _d->drawStr(2, 94, recent);

    char snippet[24];
    std::strncpy(snippet, last->text, sizeof(snippet)-1);
    snippet[sizeof(snippet)-1] = '\0';
    _d->drawStr(2, 106, snippet);
  } else {
    _d->drawStr(2, 94, "Last: none");
  }
}

void Ui::drawSettings() {
  int w = _d->width();

  _d->setFontUi();
  _d->drawStr(2, 12, "SETTINGS");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();

  char line[28];

  snprintf(line, sizeof(line), "Freq:%lu", (unsigned long)APP_LORA_FREQ_HZ);
  _d->drawStr(2, 30, line);

  snprintf(line, sizeof(line), "BW:%d kHz", (int)APP_LORA_BW_KHZ);
  _d->drawStr(2, 42, line);

  snprintf(line, sizeof(line), "SF:%d", (int)APP_LORA_SF);
  _d->drawStr(2, 54, line);

  snprintf(line, sizeof(line), "TX:%d dBm", (int)APP_LORA_TX_DBM);
  _d->drawStr(2, 66, line);

  _d->drawStr(2, 84, "Later: edit via rotary.");
}

size_t Ui::pendingCount() const {
  size_t n = 0;
  for (size_t i = 0; i < kMaxPending; i++) {
    if (_pending[i].active) n++;
  }
  return n;
}
