#pragma once
#include <stdint.h>

static constexpr uint8_t APP_PROTO_VER = 1;

enum class AppMsgType : uint8_t {
  ChatMsg = 1,
  Ack     = 2,
  Hello   = 3, // optional presence, not used yet
};

#pragma pack(push, 1)
struct WireChatPacket {
  uint8_t proto = APP_PROTO_VER;
  uint8_t type  = (uint8_t)AppMsgType::ChatMsg;
  uint16_t flags = 0;
  uint32_t msgId = 0;
  uint32_t tsMs = 0;
  uint8_t textLen = 0;
  char text[96] = {0};
};
#pragma pack(pop)
