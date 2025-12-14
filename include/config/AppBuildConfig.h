#pragma once

// AppBuildConfig.h
// Central build-time configuration. All options are expected to be provided via
// PlatformIO build_flags (per-environment), with sane defaults for development.

// -------- MCU selection (one of these should be defined by env) --------
// APP_MCU_ESP32S3, APP_MCU_ESP32C3, ...

// -------- Pinmap selection (one should be defined by env) --------
// APP_PINMAP_SUPERMINI_S3, APP_PINMAP_GENERIC_C3, ...

// -------- Display selection --------
// APP_DISPLAY_OLED or APP_DISPLAY_NONE
// For OLED: APP_OLED_CTRL (0 auto / 1306 / 1315), APP_OLED_ROT (0..3), APP_OLED_I2C_ADDR (0 auto / 0x3C / 0x3D)

// -------- Feature toggles --------
// APP_ENABLE_ROTARY (0/1)

#include <Arduino.h>

// --------------------
// Pinmaps
// --------------------
#if defined(APP_PINMAP_SUPERMINI_S3)
  #include "pinmaps/pinmap_supermini_s3.h"
#elif defined(APP_PINMAP_GENERIC_C3)
  #include "pinmaps/pinmap_generic_c3.h"
#else
  #error "No pinmap selected. Define one of: APP_PINMAP_SUPERMINI_S3, APP_PINMAP_GENERIC_C3, ..."
#endif

// --------------------
// Defaults (can be overridden by env build_flags)
// --------------------
#ifndef APP_LOG_BAUD
  #define APP_LOG_BAUD 115200
#endif

#ifndef APP_I2C_CLOCK_HZ
  #define APP_I2C_CLOCK_HZ 100000
#endif

#ifndef APP_OLED_CTRL
  #define APP_OLED_CTRL 0
#endif

#ifndef APP_OLED_I2C_ADDR
  #define APP_OLED_I2C_ADDR 0
#endif

#ifndef APP_OLED_ROT
  // Default to portrait CW (U8G2_R1) for DIY handheld nodes
  #define APP_OLED_ROT 1
#endif

#ifndef APP_ENABLE_ROTARY
  #define APP_ENABLE_ROTARY 0
#endif

// LoRa defaults
#ifndef APP_LORA_FREQ_HZ
  #define APP_LORA_FREQ_HZ 433000000
#endif

#ifndef APP_LORA_BW_KHZ
  #define APP_LORA_BW_KHZ 125
#endif

#ifndef APP_LORA_SF
  #define APP_LORA_SF 12
#endif

#ifndef APP_LORA_TX_DBM
  #define APP_LORA_TX_DBM 17
#endif

#ifndef APP_LORA_SYNCWORD
  #define APP_LORA_SYNCWORD 0x12
#endif

#ifndef APP_LORA_PREAMBLE
  #define APP_LORA_PREAMBLE 8
#endif

// --------------------
// Sanity checks
// --------------------
#if (defined(APP_DISPLAY_OLED) + defined(APP_DISPLAY_TFT) + defined(APP_DISPLAY_NONE)) > 1
  #error "Select exactly one display type: APP_DISPLAY_OLED / APP_DISPLAY_TFT / APP_DISPLAY_NONE"
#endif

#if !defined(APP_DISPLAY_OLED) && !defined(APP_DISPLAY_TFT) && !defined(APP_DISPLAY_NONE)
  // Default: headless (safe)
  #define APP_DISPLAY_NONE 1
#endif

#if (APP_ENABLE_ROTARY == 1) && defined(APP_DISPLAY_NONE)
  // Rotary without a screen is supported, but you probably want to disable rotary in that case.
#endif

// Helpers
constexpr uint32_t app_lora_freq_hz() { return (uint32_t)APP_LORA_FREQ_HZ; }
constexpr float app_lora_freq_mhz() { return (float)APP_LORA_FREQ_HZ / 1000000.0f; }
constexpr float app_lora_bw_khz() { return (float)APP_LORA_BW_KHZ; }
constexpr int app_lora_sf() { return (int)APP_LORA_SF; }
constexpr int app_lora_tx_dbm() { return (int)APP_LORA_TX_DBM; }
constexpr int app_lora_syncword() { return (int)APP_LORA_SYNCWORD; }
constexpr int app_lora_preamble() { return (int)APP_LORA_PREAMBLE; }