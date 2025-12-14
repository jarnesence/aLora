#pragma once
#include <stdint.h>
#include <stddef.h>
#include "net/NetTypes.h"
#include "storage/Settings.h"

class MeshRadio;
class ChatLog;

struct PendingMsg {
  bool used = false;
  uint16_t dst = 0;
  uint32_t msgId = 0;
  uint8_t tries = 0;
  uint32_t nextSendAtMs = 0;
  char text[96] = {0};
};

class ReliableSender {
public:
  void begin(MeshRadio* r, ChatLog* log, const AppSettings* s);
  void tick();

  // Enqueue a chat message for send
  bool sendChat(uint16_t dst, const char* text);

  // Called when ACK is received
  void onAck(uint16_t from, uint32_t msgId);

private:
  uint32_t nowMs() const;

  MeshRadio* _r = nullptr;
  ChatLog* _log = nullptr;
  const AppSettings* _s = nullptr;

  PendingMsg _q[4];
};
