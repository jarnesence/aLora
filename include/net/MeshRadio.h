#pragma once
#include <stdint.h>
#include <stddef.h>
#include "net/Packets.h"
#include "net/Dedupe.h"

// Thin wrapper around LoRaMesher.
// Responsibilities:
// - Configure LoRaMesher pins + radio parameters from AppBuildConfig.
// - Provide a single RX callback delivering decoded WireChatPacket.
// - Provide a DM send primitive (currently uses LoRaMesher reliable send).
class MeshRadio {
public:
  using RxCallback = void (*)(uint16_t src, const WireChatPacket& pkt, int16_t rssi, float snr);

  bool begin();
  void setRxCallback(RxCallback cb);

  bool sendDm(uint16_t dst, const WireChatPacket& pkt);

  uint16_t localAddress() const;

  uint32_t rxCount() const { return _rxCount; }
  uint32_t txCount() const { return _txCount; }

private:
  static void processReceivedPackets(void* pv);

  void onRxPacket(uint16_t src, const WireChatPacket& pkt, int16_t rssi, float snr);

  void* _rxTaskHandle = nullptr; // TaskHandle_t (kept void* to avoid extra includes in header)
  volatile uint32_t _rxCount = 0;
  volatile uint32_t _txCount = 0;
  RxCallback _rxCb = nullptr;
  mutable DedupeCache _dedupe;
  uint32_t _msgSeq = 1;
};
