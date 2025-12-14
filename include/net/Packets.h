#pragma once
#include <stdint.h>

// Wire format for direct messages and delivery receipts.
// Keep it small: large payloads increase airtime and collision probability.
enum class PacketKind : uint8_t { Chat = 0, Ack = 1 };

struct WireChatPacket {
  PacketKind kind;       // Chat or Ack
  uint32_t msgId;        // Sender-generated id (monotonic or random)
  uint16_t to;           // Destination node
  uint16_t from;         // Source node (optional; can be overwritten by receiver metadata)
  uint32_t ts;           // Unix time if available, else millis()/1000
  uint32_t refMsgId;     // For Ack: original chat msgId; for Chat: 0
  char text[92];         // Null-terminated UTF-8 subset (ASCII recommended for now)
};
