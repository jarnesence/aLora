#pragma once

// pinmap_supermini_s3.h
// DIY ESP32-S3 SuperMini + OLED (I2C) + LoRa (RA-01/RA-02, SX127x family) + EC11 Rotary
//
// Notes:
// - Keep this file strictly about pins. Feature toggles belong in platformio.ini build_flags.
// - All comments are in English to keep the project GitHub-friendly.

#ifndef APP_I2C_SDA
  #define APP_I2C_SDA 5
#endif
#ifndef APP_I2C_SCL
  #define APP_I2C_SCL 4
#endif
#ifndef APP_OLED_RESET
  #define APP_OLED_RESET 16
#endif

// Rotary Encoder (EC11)
#ifndef APP_ROTARY_PIN_A
  #define APP_ROTARY_PIN_A 6
#endif
#ifndef APP_ROTARY_PIN_B
  #define APP_ROTARY_PIN_B 7
#endif
#ifndef APP_ROTARY_PIN_BTN
  #define APP_ROTARY_PIN_BTN 2
#endif

// LoRa SPI
#ifndef APP_LORA_SCK
  #define APP_LORA_SCK 12
#endif
#ifndef APP_LORA_MISO
  #define APP_LORA_MISO 13
#endif
#ifndef APP_LORA_MOSI
  #define APP_LORA_MOSI 11
#endif
#ifndef APP_LORA_CS
  #define APP_LORA_CS 10
#endif

// LoRa DIO/IRQ pins
#ifndef APP_LORA_DIO0
  #define APP_LORA_DIO0 8
#endif

// Some modules use DIO1 (SX126x). For SX127x you can keep it equal to DIO0 to satisfy APIs that require it.
#ifndef APP_LORA_DIO1
  #define APP_LORA_DIO1 APP_LORA_DIO0
#endif

#ifndef APP_LORA_RST
  #define APP_LORA_RST 9
#endif

// Battery sense (optional; not used yet by default UI)
#ifndef APP_BATTERY_ADC_PIN
  #define APP_BATTERY_ADC_PIN 1
#endif
