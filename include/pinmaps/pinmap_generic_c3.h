#pragma once

// pinmap_generic_c3.h
// Placeholder pinmap for ESP32-C3 boards. Adjust to your wiring.
//
// This pinmap intentionally disables display/rotary by default in platformio.ini.
#ifndef APP_LORA_SCK
  #define APP_LORA_SCK 4
#endif
#ifndef APP_LORA_MISO
  #define APP_LORA_MISO 5
#endif
#ifndef APP_LORA_MOSI
  #define APP_LORA_MOSI 6
#endif
#ifndef APP_LORA_CS
  #define APP_LORA_CS 7
#endif
#ifndef APP_LORA_DIO0
  #define APP_LORA_DIO0 2
#endif
#ifndef APP_LORA_DIO1
  #define APP_LORA_DIO1 APP_LORA_DIO0
#endif
#ifndef APP_LORA_RST
  #define APP_LORA_RST 3
#endif
