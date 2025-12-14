#pragma once
#include <stdint.h>

// Wire format for direct messages.
// Keep it small: large payloads increase airtime and collision probability.
struct WireChatPacket {
  uint32_t msgId;        // Sender-generated id (monotonic or random)
  uint16_t to;           // Destination node
  uint16_t from;         // Source node (optional; can be overwritten by receiver metadata)
  uint32_t ts;           // Unix time if available, else millis()/1000
  char text[96];         // Null-terminated UTF-8 subset (ASCII recommended for now)
};
