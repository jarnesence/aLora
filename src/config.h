#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Hardware Pin Definitions (ESP32-S3 Super Mini) ---

// I2C (OLED)
#define PIN_I2C_SDA 5
#define PIN_I2C_SCL 4

// Rotary Encoder
#define PIN_ROT_A   6
#define PIN_ROT_B   7
#define PIN_ROT_BTN 2

// LoRa (SX1278) - Values match platformio.ini build flags
// Usually we should use the build flags if defined, or defaults here.
#ifndef PIN_LORA_SCK
#define PIN_LORA_SCK   12
#endif
#ifndef PIN_LORA_MISO
#define PIN_LORA_MISO  13
#endif
#ifndef PIN_LORA_MOSI
#define PIN_LORA_MOSI  11
#endif
#ifndef PIN_LORA_CS
#define PIN_LORA_CS    10
#endif
#ifndef PIN_LORA_DIO0
#define PIN_LORA_DIO0  8
#endif
#ifndef PIN_LORA_RST
#define PIN_LORA_RST   9
#endif

// Power Management
#define PIN_BATTERY    1

// --- LoRa Configuration ---
#define LORA_FREQ      433.0 // MHz
#define LORA_BW        125.0 // kHz
#define LORA_SF        9
#define LORA_CR        7
#define LORA_SYNC_WORD 0x12
#define LORA_POWER     20    // dBm

// --- Display Configuration ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1     // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#endif // CONFIG_H
