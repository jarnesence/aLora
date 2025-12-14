#pragma once
#include <stdint.h>
#include <stddef.h>
#include "net/NetTypes.h"

// Thin wrapper around LoRaMesher.
class MeshRadio {
public:
  using RxCallback = void (*)(uint16_t src, const WireChatPacket& pkt, int16_t rssi, float snr);

  bool begin();
  void setRxCallback(RxCallback cb);

  bool sendDm(uint16_t dst, const WireChatPacket& pkt);

  uint32_t rxCount() const { return _rxCount; }
  uint32_t txCount() const { return _txCount; }

private:
  static void receiveTask(void* pv);
  void processReceivedPackets();

  RxCallback _rxCb = nullptr;
  volatile uint32_t _rxCount = 0;
  volatile uint32_t _txCount = 0;
};
