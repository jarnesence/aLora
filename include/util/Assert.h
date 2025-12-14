#pragma once
#include <Arduino.h>

// Minimal assertion helper.
// In release builds, it prints and resets to avoid undefined behavior.

inline void appAssert(bool cond, const char* msg) {
  if (!cond) {
    Serial.printf("[ASSERT] %s\n", msg);
    delay(200);
    ESP.restart();
  }
}
