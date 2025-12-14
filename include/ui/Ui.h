#pragma once
#include <stdint.h>

#include "display/Display.h"
#include "input/Rotary.h"
#include "net/MeshRadio.h"
#include "storage/ChatLog.h"

class Ui {
public:
  Ui(IDisplay* display, RotaryInput* input, MeshRadio* radio, ChatLog* log);

  void begin();
  void tick();

  // Called from radio RX callback
  void onIncoming(uint16_t src, const WireChatPacket& pkt);

private:
  enum class Screen : uint8_t { Chat=0, Compose=1, Status=2, Settings=3 };
  enum class ComposeFocus : uint8_t { Destination=0, Cursor=1, Character=2, Send=3 };

  IDisplay* _d = nullptr;
  RotaryInput* _in = nullptr;
  MeshRadio* _radio = nullptr;
  ChatLog* _log = nullptr;

  Screen _screen = Screen::Chat;
  uint32_t _lastDrawMs = 0;

  // Chat scroll
  int32_t _chatScroll = 0;

  // Compose state
  uint16_t _dst = 1;
  char _draft[25] = {0};
  uint8_t _cursor = 0;
  uint8_t _charIndex = 0;
  ComposeFocus _focus = ComposeFocus::Destination;
  uint32_t _nextMsgId = 1;

  void drawChat();
  void drawCompose();
  void drawStatus();
  void drawSettings();

  void sendAck(uint16_t dst, uint32_t refMsgId);

  void handleInput();
  void handleClick();
  void handleDelta(int32_t delta);
  void handleComposeClick();
  void handleComposeDelta(int32_t delta);
  void advanceCursor(int32_t delta);
  void syncCharIndexToDraft();
  void sendDraft();
};
