#pragma once

// AutoConfig.h
// This header is force-included for every compilation unit via platformio.ini.
//
// Purpose:
// - Make the selected board pinout (APP_* macros) visible to third-party libraries
//   that are configured via preprocessor macros (notably LoRaMesher).
//
// IMPORTANT:
// - Keep this file lightweight and macro-only. Do not include Arduino headers here.

#include "AppBuildConfig.h"

// -----------------------------------------------------------------------------
// LoRaMesher pin mapping
// -----------------------------------------------------------------------------
// LoRaMesher 0.0.11 (the Arduino baseline release) builds its RadioLib module
// internally. It expects common pin macros to be defined at compile time.
// We map our app-level APP_* pins to the names that LoRaMesher/RadioLib expects.

#if !defined(LORA_SCK) && defined(APP_LORA_SCK)
  #define LORA_SCK APP_LORA_SCK
#endif

#if !defined(LORA_MISO) && defined(APP_LORA_MISO)
  #define LORA_MISO APP_LORA_MISO
#endif

#if !defined(LORA_MOSI) && defined(APP_LORA_MOSI)
  #define LORA_MOSI APP_LORA_MOSI
#endif

#if !defined(LORA_CS) && defined(APP_LORA_CS)
  #define LORA_CS APP_LORA_CS
#endif

#if !defined(LORA_RST) && defined(APP_LORA_RST)
  #define LORA_RST APP_LORA_RST
#endif

// DIO0 / IRQ
#if !defined(LORA_IRQ) && defined(APP_LORA_DIO0)
  #define LORA_IRQ APP_LORA_DIO0
#endif

// Optional DIO1 (some boards wire DIO1 to the MCU; many don't)
#if !defined(LORA_IO1) && defined(APP_LORA_DIO1)
  #define LORA_IO1 APP_LORA_DIO1
#endif
