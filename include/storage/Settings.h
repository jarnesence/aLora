#pragma once
#include <stdint.h>

// Settings.h
// Persistent settings are stored via Preferences.
// Compile-time defaults are taken from platformio.ini macros (APP_DEFAULT_*).

#include "config/AppBuildConfig.h"

struct RadioSettings {
#ifdef APP_DEFAULT_FREQ_HZ
  uint32_t frequencyHz = (uint32_t)APP_DEFAULT_FREQ_HZ;
#else
  uint32_t frequencyHz = 433000000UL;
#endif

#ifdef APP_DEFAULT_BW_KHZ
  float bandwidthKhz = (float)APP_DEFAULT_BW_KHZ;
#else
  float bandwidthKhz = 125.0f;
#endif

#ifdef APP_DEFAULT_SF
  uint8_t spreadingFactor = (uint8_t)APP_DEFAULT_SF;
#else
  uint8_t spreadingFactor = 12;
#endif

#ifdef APP_DEFAULT_SYNC_WORD
  uint8_t syncWord = (uint8_t)APP_DEFAULT_SYNC_WORD;
#else
  uint8_t syncWord = 0x12;
#endif

#ifdef APP_DEFAULT_TX_POWER_DBM
  int8_t txPowerDbm = (int8_t)APP_DEFAULT_TX_POWER_DBM;
#else
  int8_t txPowerDbm = 20;
#endif

#ifdef APP_DEFAULT_PREAMBLE_LEN
  uint16_t preambleLen = (uint16_t)APP_DEFAULT_PREAMBLE_LEN;
#else
  uint16_t preambleLen = 8;
#endif
};

struct UiSettings {
#if defined(APP_OLED_PORTRAIT) && (APP_OLED_PORTRAIT == 1)
  bool portrait = true;
#else
  bool portrait = false;
#endif
  uint8_t brightness = 255; // OLED is usually fixed; kept for TFT later.
};

struct AppSettings {
  RadioSettings radio;
  UiSettings ui;

  bool ackReliability = true;   // App-level ACK + retry
  uint8_t maxRetries = 4;
  uint16_t ackTimeoutMs = 1800;
};
