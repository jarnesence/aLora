#include <Arduino.h>
#include <cstring>
#include "config/AppBuildConfig.h"
#include "ui/Ui.h"
#include "util/Crypto.h"

static const char* kCharset = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-_/@";
static const uint8_t kGroupStarts[] = {0, 1, 27, 37};
static const char* kShortcuts[] = {"OK", "ACK", "OMW", "PING?", "HERE"};
static constexpr size_t kShortcutCount = sizeof(kShortcuts) / sizeof(kShortcuts[0]);

Ui::Ui(IDisplay* display, RotaryInput* input, MeshRadio* radio, ChatLog* log, PairingStore* pairs)
: _d(display), _in(input), _radio(radio), _log(log), _pairs(pairs) {}

static uint8_t findGroupBoundary(uint8_t current, bool forward) {
  size_t groups = sizeof(kGroupStarts) / sizeof(kGroupStarts[0]);
  if (forward) {
    for (size_t i = 0; i < groups; i++) {
      if (kGroupStarts[i] > current) return kGroupStarts[i];
    }
    return kGroupStarts[0];
  }

  // backward search
  uint8_t best = kGroupStarts[groups - 1];
  for (size_t i = 0; i < groups; i++) {
    if (kGroupStarts[i] < current) best = kGroupStarts[i];
  }
  return best;
}

void Ui::begin() {
  _lastDrawMs = 0;
  if (_draft[0] == '\0') std::strcpy(_draft, "ALORA READY");
  syncCharIndexToDraft();
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
  int32_t delta = _in->readDelta();

  if (click) handleClick();
  if (delta != 0) handleDelta(delta);
}

void Ui::handleClick() {
  if (_screen == Screen::Compose) {
    handleComposeClick();
    return;
  }

  if (_screen == Screen::Chat) _screen = Screen::Contacts;
  else if (_screen == Screen::Contacts) {
    selectContactTarget();
    _screen = Screen::Compose;
  }
  else if (_screen == Screen::Status) _screen = Screen::Settings;
  else if (_screen == Screen::Settings) _screen = Screen::Chat;
  else _screen = Screen::Chat;
}

void Ui::handleDelta(int32_t delta) {
  switch (_screen) {
    case Screen::Chat: {
      _chatScroll += (delta > 0 ? 1 : -1);
      if (_chatScroll < 0) _chatScroll = 0;
      int maxScroll = (int)filteredChatCount() - 1;
      if (maxScroll < 0) maxScroll = 0;
      if (_chatScroll > maxScroll) _chatScroll = maxScroll;
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
    case Screen::Status:
    case Screen::Settings:
    default:
      break;
  }
}

void Ui::handleComposeClick() {
  switch (_focus) {
    case ComposeFocus::Destination:
      _focus = (contactListLength() > 1) ? ComposeFocus::Contact : ComposeFocus::Cursor;
      break;
    case ComposeFocus::Contact:
      selectContactTarget();
      _focus = ComposeFocus::Cursor;
      break;
    case ComposeFocus::Cursor:
      _focus = ComposeFocus::Character;
      break;
    case ComposeFocus::Character:
      _focus = ComposeFocus::Shortcut;
      break;
    case ComposeFocus::Shortcut:
      applyShortcut();
      _focus = ComposeFocus::Send;
      break;
    case ComposeFocus::Send:
      sendDraft();
      _focus = ComposeFocus::Destination;
      _screen = Screen::Chat;
      break;
  }
}

void Ui::handleComposeDelta(int32_t delta) {
  switch (_focus) {
    case ComposeFocus::Destination: {
      int32_t nd = (int32_t)_dst + delta;
      if (nd < 1) nd = 1;
      if (nd > 65535) nd = 65535;
      _dst = (uint16_t)nd;
      break;
    }
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
    case ComposeFocus::Cursor:
      advanceCursor(delta);
      syncCharIndexToDraft();
      break;
    case ComposeFocus::Character: {
      size_t n = std::strlen(kCharset);
      int32_t idx = 0;
      if (delta > 3 || delta < -3) {
        idx = (int32_t)findGroupBoundary(_charIndex, delta > 0);
      } else {
        idx = (int32_t)_charIndex + delta;
        while (idx < 0) idx += (int32_t)n;
        idx %= (int32_t)n;
      }
      _charIndex = (uint8_t)idx;

      _draft[_cursor] = kCharset[_charIndex];
      _draft[sizeof(_draft)-1] = '\0';
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

void Ui::advanceCursor(int32_t delta) {
  int32_t next = (int32_t)_cursor + delta;
  int32_t maxIdx = (int32_t)sizeof(_draft) - 2; // leave room for null
  if (next < 0) next = 0;
  if (next > maxIdx) next = maxIdx;
  _cursor = (uint8_t)next;

  if (_draft[_cursor] == '\0') {
    _draft[_cursor] = ' ';
    _draft[_cursor + 1] = '\0';
  }
}

void Ui::syncCharIndexToDraft() {
  char c = _draft[_cursor];
  if (c == '\0') c = ' ';

  const char* pos = std::strchr(kCharset, c);
  if (pos) {
    _charIndex = (uint8_t)(pos - kCharset);
  } else {
    _charIndex = 0;
  }
}

void Ui::applyShortcut() {
  if (kShortcutCount == 0) return;
  const char* phrase = kShortcuts[_shortcutIdx % kShortcutCount];
  size_t phraseLen = ::strnlen(phrase, sizeof(_draft));
  size_t room = sizeof(_draft) - 1;
  size_t curLen = ::strnlen(_draft, room);
  if (_cursor > curLen) _cursor = (uint8_t)curLen;
  if (phraseLen > room - _cursor) phraseLen = room - _cursor;

  size_t movable = (curLen > _cursor) ? (curLen - _cursor) : 0;
  if (movable > room - _cursor - phraseLen) movable = room - _cursor - phraseLen;

  std::memmove(_draft + _cursor + phraseLen, _draft + _cursor, movable + 1); // include null
  std::memcpy(_draft + _cursor, phrase, phraseLen);
  _cursor += (uint8_t)phraseLen;
  _draft[room] = '\0';
  syncCharIndexToDraft();
}

void Ui::selectContactTarget() {
  SeenPeer peer{};
  if (!contactAt(_contactSelection, peer)) return;

  if (peer.addr == 0) {
    _chatPeerFilter = 0;
    return;
  }

  _dst = peer.addr;
  _chatPeerFilter = peer.addr;
  recordSeenPeer(peer.addr, peer.paired);

  int maxScroll = (int)filteredChatCount() - 1;
  if (maxScroll < 0) maxScroll = 0;
  if (_chatScroll > maxScroll) _chatScroll = maxScroll;
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

void Ui::handlePresence(uint16_t src, const WireChatPacket& pkt) {
  uint16_t me = _radio ? _radio->localAddress() : 0;
  if (pkt.to != 0xFFFF && pkt.to != me) return;
  notePairing(src, "Presence seen.");
}

void Ui::handlePairRequest(uint16_t src, const WireChatPacket& pkt) {
  uint16_t me = _radio ? _radio->localAddress() : 0;
  if (pkt.to != me && pkt.to != 0xFFFF) return;
  if (!_pairs) return;

  uint8_t key[PairingStore::kKeyLen] = {0};
  uint32_t acceptNonce = nextNonce();
  if (_pairs->deriveFromRequest(src, pkt.msgId, pkt.nonce, acceptNonce, key)) {
    notePairing(src, "Paired via incoming request.");
    recordSeenPeer(src, true);
  }
  sendPairAccept(src, pkt, acceptNonce);
}

void Ui::handlePairAccept(uint16_t src, const WireChatPacket& pkt) {
  uint16_t me = _radio ? _radio->localAddress() : 0;
  if (pkt.to != me && pkt.to != 0xFFFF) return;
  if (!_pairs) return;

  uint8_t key[PairingStore::kKeyLen] = {0};
  if (_pairs->resolvePendingRequest(src, pkt.refMsgId, pkt.nonce, key)) {
    notePairing(src, "Pairing accepted.");
    recordSeenPeer(src, true);
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
  pkt.nonce = nextNonce();
  pkt.textLen = 0;
  pkt.reserved = 0;
  std::strncpy(pkt.text, "PAIR_REQ", sizeof(pkt.text) - 1);

  _radio->sendDm(dst, pkt);
  _pairs->recordOutgoingRequest(dst, pkt.msgId, pkt.nonce);
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
  // include "All" row
  return contactCount() + 1;
}

bool Ui::contactAt(size_t idx, SeenPeer& out) const {
  if (idx == 0) {
    out.active = true;
    out.addr = 0;
    out.paired = false;
    out.lastSeenSec = 0;
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

bool Ui::matchesChatFilter(const ChatMsg& m) const {
  if (_chatPeerFilter == 0) return true;
  return m.src == _chatPeerFilter;
}

size_t Ui::filteredChatCount() const {
  if (!_log) return 0;
  size_t total = 0;
  size_t sz = _log->size();
  for (size_t i = 0; i < sz; i++) {
    if (matchesChatFilter(_log->at(i))) total++;
  }
  return total;
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
  if (now - _lastDrawMs < 100) return;
  _lastDrawMs = now;

  if (!_d) return;
  _d->clear();

  switch (_screen) {
    case Screen::Chat: drawChat(); break;
    case Screen::Contacts: drawContacts(); break;
    case Screen::Compose: drawCompose(); break;
    case Screen::Status: drawStatus(); break;
    case Screen::Settings: drawSettings(); break;
  }

  _d->send();
}

void Ui::drawChat() {
  int w = _d->width();
  int h = _d->height();

  _d->setFontUi();
  char title[28];
  if (_chatPeerFilter == 0) snprintf(title, sizeof(title), "CHAT (click=contacts)");
  else snprintf(title, sizeof(title), "CHAT %u", (unsigned)_chatPeerFilter);
  _d->drawStr(2, 12, title);
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();

  size_t matchCount = filteredChatCount();
  if (matchCount == 0) {
    _d->drawStr(2, 32, "No messages yet.");
    _d->drawStr(2, 44, "Rotate to refresh list.");
    return;
  }

  int y = 28;
  int lineH = 12;
  int total = (int)_log->size();

  int idxNewest = -1;
  size_t seen = 0;
  for (int i = total - 1; i >= 0; --i) {
    const ChatMsg& m = _log->at((size_t)i);
    if (!matchesChatFilter(m)) continue;
    if (seen == (size_t)_chatScroll) { idxNewest = i; break; }
    seen++;
  }

  if (idxNewest < 0) idxNewest = total - 1;

  for (int lines = 0; lines < 8 && y < h; ) {
    if (idxNewest < 0) break;

    const ChatMsg& m = _log->at((size_t)idxNewest);
    idxNewest--;
    if (!matchesChatFilter(m)) continue;

    const char* state = "";
    if (m.outgoing) {
      if (m.failed) state = "!";
      else state = m.delivered ? "✓" : "…";
    }

    char header[18];
    snprintf(header, sizeof(header), "%c%u%s:", m.outgoing ? '>' : '<', (unsigned)m.src, state);
    _d->drawStr(2, y, header);

    // Show a compact snippet; for portrait width it's very limited.
    char snippet[18];
    std::strncpy(snippet, m.text, sizeof(snippet)-1);
    snippet[sizeof(snippet)-1] = '\0';

    _d->drawStr(2, y + 10, snippet);

    y += (lineH * 2);
    lines++;
  }
}

void Ui::drawContacts() {
  int w = _d->width();
  _d->setFontUi();
  _d->drawStr(2, 12, "CONTACTS (click=set)");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();
  size_t len = contactListLength();
  if (len <= 1) {
    _d->drawStr(2, 32, "No peers yet.");
    _d->drawStr(2, 46, "Rotate after beacons.");
    return;
  }

  uint32_t now = (uint32_t)(millis() / 1000);
  int lineH = 12;
  int yBase = 28;

  for (size_t row = 0; row < 6; row++) {
    size_t idx = (size_t)_contactScroll + row;
    if (idx >= len) break;

    SeenPeer peer{};
    if (!contactAt(idx, peer)) continue;

    char line[32];
    if (peer.addr == 0) {
      std::snprintf(line, sizeof(line), "[ALL CHATS]");
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

void Ui::drawCompose() {
  int w = _d->width();

  _d->setFontUi();
  _d->drawStr(2, 12, "COMPOSE (click=cycle)");
  _d->drawHLine(0, 14, w);

  _d->setFontSmall();

  char line1[24];
  snprintf(line1, sizeof(line1), "To: %u", (unsigned)_dst);
  _d->drawStr(2, 28, line1);

  const char* focusLabel = "DST";
  if (_focus == ComposeFocus::Contact) focusLabel = "CNT";
  else if (_focus == ComposeFocus::Cursor) focusLabel = "CUR";
  else if (_focus == ComposeFocus::Character) focusLabel = "CHAR";
  else if (_focus == ComposeFocus::Shortcut) focusLabel = "SHOT";
  else if (_focus == ComposeFocus::Send) focusLabel = "SEND";

  char focusLine[28];
  snprintf(focusLine, sizeof(focusLine), "Focus:%s", focusLabel);
  _d->drawStr(2, 40, focusLine);

  SeenPeer sel{};
  contactAt(_contactSelection, sel);
  char contactLine[28];
  if (sel.addr == 0) snprintf(contactLine, sizeof(contactLine), "Contact:All chats");
  else snprintf(contactLine, sizeof(contactLine), "Contact:%u%s", (unsigned)sel.addr, sel.paired ? "*" : "");
  _d->drawStr(2, 52, contactLine);

  char cursorLine[28];
  snprintf(cursorLine, sizeof(cursorLine), "Cursor:%u", (unsigned)_cursor);
  _d->drawStr(2, 64, cursorLine);

  char shortcutLine[28];
  snprintf(shortcutLine, sizeof(shortcutLine), "Shortcut:%s", kShortcuts[_shortcutIdx % kShortcutCount]);
  _d->drawStr(2, 76, shortcutLine);

  _d->drawStr(2, 92, "Text:");
  _d->drawFrame(0, 96, w, 28);

  // Render the draft (one line for now)
  char shown[18];
  std::strncpy(shown, _draft, sizeof(shown)-1);
  shown[sizeof(shown)-1] = '\0';
  _d->drawStr(2, 108, shown);

  _d->drawStr(2, 122, "Click: focus/send");
}

void Ui::drawStatus() {
  int w = _d->width();

  _d->setFontUi();
  _d->drawStr(2, 12, "STATUS (click=next)");
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

  _d->drawStr(2, 94, "Retries:bounded+discover");
  _d->drawStr(2, 106, "Broadcast only on fail.");
}

void Ui::drawSettings() {
  int w = _d->width();

  _d->setFontUi();
  _d->drawStr(2, 12, "SETTINGS (click=next)");
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
