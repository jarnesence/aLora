#pragma once

// pinmap_c3_example.h
// Example wiring for an ESP32-C3 devkit.
//
// IMPORTANT: This is an EXAMPLE ONLY. Change pins to match your wiring.

#define APP_BOARD_NAME "ESP32-C3 Example"

// I2C (OLED)
#define PIN_I2C_SDA 4
#define PIN_I2C_SCL 5
#define PIN_OLED_RESET -1  // not wired

// Rotary
#define PIN_ROT_A   6
#define PIN_ROT_B   7
#define PIN_ROT_BTN 2

// LoRa (SX127x SPI example)
#define PIN_LORA_MISO 1
#define PIN_LORA_SCK  0
#define PIN_LORA_MOSI 10
#define PIN_LORA_CS   9
#define PIN_LORA_RST  8
#define PIN_LORA_DIO0 3
#define PIN_LORA_DIO1 -1

// Battery (disabled by default in platformio.ini for this env)
#define PIN_BATTERY_ADC 2
