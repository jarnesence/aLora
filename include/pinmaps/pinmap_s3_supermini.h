#pragma once

// pinmap_s3_supermini.h
// Wiring for a DIY ESP32-S3 "Super Mini" build.
//
// This header MUST NOT enable/disable features. It only declares PIN_* macros.
// Select features and modules in platformio.ini (APP_DISPLAY_*, APP_FEATURE_*, APP_LORA_MODULE_*).

#define APP_BOARD_NAME "ESP32-S3 SuperMini"

// --------------------
// I2C (OLED)
// --------------------
// Avoid GPIOs reserved by JTAG if you keep JTAG enabled.
#ifndef USE_JTAG
  #define PIN_I2C_SDA 5
  #define PIN_I2C_SCL 4
#endif

// Optional OLED reset pin (set to an actual GPIO if wired, or omit)
#define PIN_OLED_RESET 16

// --------------------
// Rotary encoder (EC11)
// --------------------
#define PIN_ROT_A   6
#define PIN_ROT_B   7
#define PIN_ROT_BTN 2

// --------------------
// LoRa module wiring (SPI)
// --------------------
// RA-01 / RA-02 (SX1278 / SX1276 family)
// If you use SX126x (E22-xxx), you can reuse SPI pins but you MUST also wire extra lines (DIO1/BUSY) and define them.
#define PIN_LORA_MISO 13
#define PIN_LORA_SCK  12
#define PIN_LORA_MOSI 11
#define PIN_LORA_CS   10
#define PIN_LORA_RST  9

#define PIN_LORA_DIO0 8
#define PIN_LORA_DIO1 -1  // Not wired

// --------------------
// Battery measurement (ADC) - optional
// --------------------
#define PIN_BATTERY_ADC 1

// Divider/calibration multiplier (device-specific; tune later with a multimeter)
