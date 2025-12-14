#pragma once
#include <stdint.h>

#include "display/Display.h"
#include "input/Rotary.h"
#include "net/MeshRadio.h"
#include "storage/ChatLog.h"
#include "storage/Pairing.h"

class Ui {
public:
  Ui(IDisplay* display, RotaryInput* input, MeshRadio* radio, ChatLog* log, PairingStore* pairs);

  void begin();
  void tick();

  // Called from radio RX callback
  void onIncoming(uint16_t src, const WireChatPacket& pkt);

private:
  enum class Screen : uint8_t {
    Menu=0,
    Contacts=1,
    Compose=2,
    Radio=3,
    PairMode=4,
    Broadcast=5,
    Listen=6,
    CodeEntry=7,
    ContactActions=8,
    Status=9,
    Settings=10
  };
  enum class ComposeFocus : uint8_t {
    Contact=0,
    Shortcut=1,
    Send=2
  };

  IDisplay* _d = nullptr;
  RotaryInput* _in = nullptr;
  MeshRadio* _radio = nullptr;
  ChatLog* _log = nullptr;
  PairingStore* _pairs = nullptr;

  struct PendingSend {
    bool active = false;
    uint16_t dst = 0;
    uint8_t attempts = 0;
    bool discoverySent = false;
    uint32_t lastSendMs = 0;
    uint32_t nextSendMs = 0;
    WireChatPacket pkt{};
  };

  static constexpr size_t kMaxPending = 4;
  PendingSend _pending[kMaxPending];

  Screen _screen = Screen::Status;
  uint32_t _lastDrawMs = 0;
  uint32_t _lastPresenceMs = 0;

  uint8_t _menuIndex = 0;
  uint8_t _pairingOption = 0;

  // Radio tuning presented in UI (does not rewrite runtime config yet)
  uint32_t _uiFreqHz = APP_LORA_FREQ_HZ;
  int8_t _uiBwKHz = APP_LORA_BW_KHZ;
  uint8_t _uiSf = APP_LORA_SF;
  int8_t _uiTxDbm = APP_LORA_TX_DBM;
  uint8_t _radioField = 0;

  // Pairing UX helpers
  bool _beaconActive = false;
  bool _listeningForPairs = false;
  uint32_t _beaconStartMs = 0;
  uint32_t _lastBeaconSendMs = 0;
  uint16_t _pairCandidate = 0;
  uint8_t _pairingSelection = 0;
  uint8_t _actionSelection = 0;
  uint16_t _lastJoiner = 0;
  uint16_t _pairCodeInput = 0;
  uint8_t _codeDigits[4] = {0};
  uint8_t _codeCursor = 0;
  uint32_t _pendingReqNonce = 0;
  uint32_t _pendingReqMsgId = 0;

  // Compose state
  uint16_t _dst = 1;
  char _draft[25] = {0};
  ComposeFocus _focus = ComposeFocus::Contact;
  uint8_t _contactSelection = 0;
  uint8_t _shortcutIdx = 0;
  uint32_t _nextMsgId = 1;
  uint16_t _actionTarget = 0;

  void drawMenu();
  void drawContacts();
  void drawCompose();
  void drawRadio();
  void drawPairMode();
  void drawBroadcast();
  void drawListen();
  void drawCodeEntry();
  void drawContactActions();
  void drawStatus();
  void drawSettings();

  const ChatMsg* latestMessage(uint16_t filter) const;

  void sendAck(uint16_t dst, uint32_t refMsgId);

  void handleInput();
  void handleClick();
  void handleHold();
  void handleDelta(int32_t delta);
  void handleComposeClick();
  void handleComposeDelta(int32_t delta);
  void handleRadioDelta(int32_t delta);
  void handleCodeDelta(int32_t delta);
  void selectContactTarget();
  void sendDraft();
  void sendSecureDraft();
  void sendPairRequest(uint16_t dst);
  void sendPairAccept(uint16_t dst, const WireChatPacket& req, uint32_t acceptNonce);
  void notePairing(uint16_t peer, const char* msg);
  void recordPending(const WireChatPacket& pkt, uint8_t initialAttempts, uint32_t firstDelayMs);
  void updateReliability();
  bool escalateDiscovery(PendingSend& slot, uint32_t now);
  void clearPending(uint32_t msgId);
  uint32_t computeRetryDelayMs(uint8_t attempt) const;
  size_t pendingCount() const;
  void maybeBroadcastPresence(uint32_t now);
  void maybeSendPairBeacon(uint32_t now);
  void handlePresence(uint16_t src, const WireChatPacket& pkt);
  void handlePairRequest(uint16_t src, const WireChatPacket& pkt);
  void handlePairAccept(uint16_t src, const WireChatPacket& pkt);
  void handleSecureChat(uint16_t src, const WireChatPacket& pkt);
  bool decryptSecureText(uint16_t src, const WireChatPacket& pkt, char* outText, size_t outLen);
  bool ensurePairedOrRequest(uint16_t dst);
  uint32_t nextNonce() const;
  uint16_t pairingCodeFor(uint16_t peer, uint32_t nonce) const;
  uint16_t codeValue() const;
  void resetCodeDigits();
  void confirmCodeEntry();
  char spinnerGlyph(uint32_t now) const;

  struct SeenPeer {
    bool active = false;
    uint16_t addr = 0;
    uint32_t lastSeenSec = 0;
    bool paired = false;
    bool isNew = false;
  };

    static constexpr size_t kMaxSeenPeers = 8;
    SeenPeer _seen[kMaxSeenPeers]{};
    uint16_t _chatPeerFilter = 0;  // 0 = all
    uint8_t _contactScroll = 0;

    void recordSeenPeer(uint16_t addr, bool paired);
    size_t contactCount() const;
    size_t contactListLength() const;
    bool contactAt(size_t idx, SeenPeer& out) const;  // idx=0 is "All"

    struct PairBeacon {
      bool active = false;
      uint16_t addr = 0;
      uint32_t lastSeenMs = 0;
    };

    static constexpr size_t kMaxBeacons = 6;
    PairBeacon _beacons[kMaxBeacons]{};
    void rememberBeacon(uint16_t addr, uint32_t now);
    size_t beaconCount() const;
    bool beaconAt(size_t idx, PairBeacon& out) const;

    struct RouteHealth {
      bool active = false;
      uint16_t dst = 0;
      uint8_t successStreak = 0;
      uint32_t lastAckMs = 0;
      uint32_t lastDiscoveryMs = 0;
    };

    static constexpr size_t kMaxRoutes = 6;
    RouteHealth _routes[kMaxRoutes]{};

    void noteDeliverySuccess(uint16_t dst);
    bool routeIsStale(uint16_t dst, uint32_t now) const;
    RouteHealth* routeFor(uint16_t dst);
    const RouteHealth* routeFor(uint16_t dst) const;
};
