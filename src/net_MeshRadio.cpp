#include <Arduino.h>
#include <SPI.h>

#include "config/AppBuildConfig.h"
#include "util/Compat.h"
#include "net/MeshRadio.h"

#include <LoraMesher.h>

static TaskHandle_t g_receiveTaskHandle = nullptr;

void MeshRadio::receiveTask(void* pv) {
  MeshRadio* self = reinterpret_cast<MeshRadio*>(pv);
  self->processReceivedPackets();
}

void MeshRadio::processReceivedPackets() {
  LoraMesher& radio = LoraMesher::getInstance();

  for (;;) {
    // Block until LoRaMesher notifies this task.
    ulTaskNotifyTake(pdPASS, portMAX_DELAY);

    while (radio.getReceivedQueueSize() > 0) {
      AppPacket<WireChatPacket>* p = radio.getNextAppPacket<WireChatPacket>();
      if (!p) break;

      // Metadata (RSSI/SNR) may or may not be available depending on LoRaMesher version.
      // In current versions, rssi/snr are typically stored in the packet base.
      int16_t rssi = 0;
      float snr = 0.0f;

      WireChatPacket pkt = *(p->payload);
      uint16_t src = p->src;

      _rxCount++;

      if (_rxCb) _rxCb(src, pkt, rssi, snr);

      // IMPORTANT: always delete packets to prevent memory leaks.
      radio.deletePacket(p);
    }
  }
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
  compat::set_lora_io1(cfg, (int)APP_LORA_DIO1);

  // Module selection
#if defined(APP_LORA_MODULE_SX1278)
  if constexpr (compat::has_module<decltype(cfg)>::value) cfg.module = LoraMesher::LoraModules::SX1278_MOD;
#elif defined(APP_LORA_MODULE_SX1276)
  if constexpr (compat::has_module<decltype(cfg)>::value) cfg.module = LoraMesher::LoraModules::SX1276_MOD;
#elif defined(APP_LORA_MODULE_SX1262)
  if constexpr (compat::has_module<decltype(cfg)>::value) cfg.module = LoraMesher::LoraModules::SX1262_MOD;
#elif defined(APP_LORA_MODULE_SX1268)
  if constexpr (compat::has_module<decltype(cfg)>::value) cfg.module = LoraMesher::LoraModules::SX1268_MOD;
#elif defined(APP_LORA_MODULE_SX1280)
  if constexpr (compat::has_module<decltype(cfg)>::value) cfg.module = LoraMesher::LoraModules::SX1280_MOD;
#else
  // Default to SX1278 (RA-01/RA-02 at 433 MHz)
  if constexpr (compat::has_module<decltype(cfg)>::value) cfg.module = LoraMesher::LoraModules::SX1278_MOD;
#endif

  // Optional LoRa params (set only if the struct exposes them)
  compat::set_freq_mhz(cfg, app_lora_freq_mhz());
  compat::set_bw_khz(cfg, app_lora_bw_khz());
  compat::set_sf(cfg, app_lora_sf());
  compat::set_power_dbm(cfg, app_lora_tx_dbm());
  compat::set_syncword(cfg, app_lora_syncword());
  compat::set_preamble(cfg, app_lora_preamble());

  radio.begin(cfg);
  radio.start();

  // Create and register receive task for app packets
  if (!g_receiveTaskHandle) {
    BaseType_t res = xTaskCreate(
      MeshRadio::receiveTask,
      "LoRaDM_RX",
      4096,
      this,
      2,
      &g_receiveTaskHandle
    );
    if (res != pdPASS) {
      Serial.printf("[MeshRadio] RX task creation failed: %d\n", (int)res);
      return false;
    }
    radio.setReceiveAppDataTaskHandle(g_receiveTaskHandle);
  }

  Serial.println("[MeshRadio] started");
  return true;
}

void MeshRadio::setRxCallback(RxCallback cb) {
  _rxCb = cb;
}

bool MeshRadio::sendDm(uint16_t dst, const WireChatPacket& pkt) {
  LoraMesher& radio = LoraMesher::getInstance();

  // Reliable send: LoRaMesher will route and retry as needed.
  radio.sendReliable(dst, (void*)&pkt, 1);
  _txCount++;
  return true;
}
