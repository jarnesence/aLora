#include "net/MeshRadio.h"

#include <Arduino.h>
#include <math.h>
#include <SPI.h>
#include <LoraMesher.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config/AppBuildConfig.h"
#include "util/Compat.h"

// Note: LoRaMesher expects an app RX task handle to notify when packets arrive.
// We implement that task as a static MeshRadio method so it can access private members.

void MeshRadio::processReceivedPackets(void* pv) {
  MeshRadio* self = reinterpret_cast<MeshRadio*>(pv);
  LoraMesher& radio = LoraMesher::getInstance();

  for (;;) {
    // Block until LoRaMesher notifies this task.
    ulTaskNotifyTake(pdPASS, portMAX_DELAY);

    while (radio.getReceivedQueueSize() > 0) {
      AppPacket<WireChatPacket>* p = radio.getNextAppPacket<WireChatPacket>();
      if (!p) break;

      // RSSI/SNR availability depends on LoRaMesher version; keep placeholders for now.
      int16_t rssi = 0;
      float snr = 0.0f;

      WireChatPacket pkt = *(p->payload);
      uint16_t src = p->src;

      self->_rxCount++;
      self->onRxPacket(src, pkt, rssi, snr);

      // IMPORTANT: always delete packets to prevent memory leaks.
      radio.deletePacket(p);
    }
  }
}

void MeshRadio::onRxPacket(uint16_t src, const WireChatPacket& pkt, int16_t rssi, float snr) {
  if (_dedupe.seen(src, pkt.msgId)) {
    if (pkt.kind == PacketKind::Chat) {
      sendAck(src, pkt.msgId);
    }
    return;
  }

  _dedupe.remember(src, pkt.msgId);
  if (_rxCb) _rxCb(src, pkt, rssi, snr);
}

void MeshRadio::sendAck(uint16_t dst, uint32_t refMsgId) {
  WireChatPacket ack{};
  ack.kind = PacketKind::Ack;
  ack.msgId = 0;  // sendDm will assign if zero
  ack.to = dst;
  ack.from = localAddress();
  ack.ts = (uint32_t)(millis() / 1000);
  ack.refMsgId = refMsgId;
  ack.text[0] = '\0';
  sendDm(dst, ack);
}

bool MeshRadio::sendDiscovery(uint16_t target, uint32_t refMsgId) {
  LoraMesher& radio = LoraMesher::getInstance();

  (void)refMsgId; // Reserved for future ref-based probes.

  WireChatPacket probe{};
  probe.kind = PacketKind::Discovery;
  probe.msgId = ++_msgSeq;
  probe.to = target;
  probe.from = localAddress();
  probe.ts = (uint32_t)(millis() / 1000);
  probe.refMsgId = 0;
  probe.text[0] = '\0';

  // Broadcast to refresh paths while keeping the intended destination in the packet.
  const uint16_t kBroadcastAddr = 0xFFFF;
  radio.sendReliable(kBroadcastAddr, &probe, 1);
  _txCount++;
  _txAirtimeMs += estimateTimeOnAirMs(sizeof(WireChatPacket));
  return true;
}

bool MeshRadio::begin() {
  // Setup SPI pins for your wiring
  SPI.begin(APP_LORA_SCK, APP_LORA_MISO, APP_LORA_MOSI, APP_LORA_CS);

  LoraMesher& radio = LoraMesher::getInstance();

  LoraMesher::LoraMesherConfig cfg = LoraMesher::LoraMesherConfig();

  // Mandatory pins
  compat::set_lora_cs(cfg, (int)APP_LORA_CS);
  compat::set_lora_rst(cfg, (int)APP_LORA_RST);
  compat::set_lora_irq(cfg, (int)APP_LORA_DIO0);

  // Optional DIO1 (if wired)
#if APP_LORA_DIO1 >= 0
  compat::set_lora_io1(cfg, (int)APP_LORA_DIO1);
#endif

  // Module selection (default SX1278 for RA-01 / RA-02)
#if defined(APP_LORA_MODULE_SX1276) && APP_LORA_MODULE_SX1276
  compat::set_module(cfg, LoraMesher::LoraModules::SX1276_MOD);
#elif defined(APP_LORA_MODULE_SX1278) && APP_LORA_MODULE_SX1278
  compat::set_module(cfg, LoraMesher::LoraModules::SX1278_MOD);
#elif defined(APP_LORA_MODULE_SX1262) && APP_LORA_MODULE_SX1262
  compat::set_module(cfg, LoraMesher::LoraModules::SX1262_MOD);
#elif defined(APP_LORA_MODULE_SX1268) && APP_LORA_MODULE_SX1268
  compat::set_module(cfg, LoraMesher::LoraModules::SX1268_MOD);
#endif

  // Radio parameters
  compat::set_freq_mhz(cfg, ((float)APP_LORA_FREQ_HZ) / 1000000.0f);
  compat::set_bw_khz(cfg, (float)APP_LORA_BW_KHZ);
  compat::set_sf(cfg, (int)APP_LORA_SF);
  compat::set_syncword(cfg, (int)APP_LORA_SYNCWORD);
  compat::set_power_dbm(cfg, (int)APP_LORA_TX_DBM);
  compat::set_preamble(cfg, (int)APP_LORA_PREAMBLE);

  // SPI object (if supported by this LoRaMesherConfig version)
  compat::set_spi(cfg, &SPI);

  // Init + start
  radio.begin(cfg);
  radio.start();

  // Create the "receive app packets" task and register it to LoRaMesher.
  TaskHandle_t handle = nullptr;
  int res = xTaskCreate(
      &MeshRadio::processReceivedPackets,
      "RxApp",
      4096,
      this,
      2,
      &handle);

  if (res != pdPASS) {
    Serial.printf("[MeshRadio] ERROR: RxApp task creation failed: %d\n", res);
    return false;
  }

  _rxTaskHandle = (void*)handle;
  radio.setReceiveAppDataTaskHandle(handle);

  Serial.println("[MeshRadio] started.");
  return true;
}

void MeshRadio::setRxCallback(RxCallback cb) {
  _rxCb = cb;
}

bool MeshRadio::sendDm(uint16_t dst, const WireChatPacket& pkt) {
  LoraMesher& radio = LoraMesher::getInstance();

  // Avoid template instantiation with T=void by passing a typed pointer.
  WireChatPacket tmp = pkt;
  if (tmp.kind != PacketKind::Ack && tmp.kind != PacketKind::Chat && tmp.kind != PacketKind::Discovery) {
    tmp.kind = PacketKind::Chat;
  }
  tmp.to = dst;
  if (tmp.msgId == 0) tmp.msgId = ++_msgSeq;
  if (tmp.from == 0) tmp.from = localAddress();
  radio.sendReliable(dst, &tmp, 1);

  _txCount++;
  _txAirtimeMs += estimateTimeOnAirMs(sizeof(WireChatPacket));
  return true;
}

uint16_t MeshRadio::localAddress() const {
  return LoraMesher::getInstance().getLocalAddress();
}

uint32_t MeshRadio::estimateTimeOnAirMs(size_t payloadBytes) const {
  // LoRa airtime (approximate): Tsym = 2^SF / BW. Payload symbols are calculated with SF, CR=4/5, and DE for SF >= 11.
  // This keeps timing deterministic and avoids dynamic allocation.
  const float bwHz = ((float)APP_LORA_BW_KHZ) * 1000.0f;
  const int sf = (int)APP_LORA_SF;
  const bool lowDataRateOpt = (bwHz <= 62500.0f) && (sf >= 11);
  const int cr = 1; // Coding rate denominator offset (4/5)

  const float tsymMs = (float)(1UL << sf) / bwHz * 1000.0f;

  const int headerEnabled = 1;
  const int de = lowDataRateOpt ? 1 : 0;
  const int payloadSym = 8 + (int)ceilf(
    (float)(8 * (int)payloadBytes - 4 * sf + 28 + 16 - 20 * headerEnabled) /
    (float)(4 * (sf - 2 * de))
  ) * (cr + 4);

  const int preambleSym = APP_LORA_PREAMBLE + 4.25f;
  float tPayloadMs = tsymMs * (float)payloadSym;
  float tPreambleMs = tsymMs * (float)preambleSym;

  uint32_t totalMs = (uint32_t)(tPayloadMs + tPreambleMs);
  return totalMs > 0 ? totalMs : 1;
}
