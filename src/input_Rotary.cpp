#include <Arduino.h>
#include "config/AppBuildConfig.h"

#if (APP_ENABLE_ROTARY == 1)
  #include <AiEsp32RotaryEncoder.h>
#endif

#include "input/Rotary.h"

#if (APP_ENABLE_ROTARY == 1)

// EC11 commonly has 20 detents. We'll handle scaling later in UI if needed.
static AiEsp32RotaryEncoder g_rotary(APP_ROTARY_PIN_A, APP_ROTARY_PIN_B, APP_ROTARY_PIN_BTN, -1, 4);

static void IRAM_ATTR readEncoderISR() {
  g_rotary.readEncoder_ISR();
}

bool RotaryInput::begin() {
  g_rotary.begin();
  g_rotary.setup(readEncoderISR);
  g_rotary.setAcceleration(0);
  g_rotary.setBoundaries(-100000, 100000, true);

  // Button: use internal pull-up
  pinMode(APP_ROTARY_PIN_BTN, INPUT_PULLUP);

  _delta = 0;
  _clicked = false;
  _held = false;
  _btnDown = false;
  _downSinceMs = 0;
  return true;
}

void RotaryInput::tick() {
  // Encoder delta
  int16_t change = g_rotary.encoderChanged();
  if (change != 0) {
    _delta += (int32_t)change;
  }

  // Button
  bool nowDown = digitalRead(APP_ROTARY_PIN_BTN) == LOW;
  uint32_t nowMs = millis();
  if (nowDown && !_btnDown) {
    _downSinceMs = nowMs;
  }
  if (!nowDown && _btnDown) {
    uint32_t heldMs = nowMs - _downSinceMs;
    if (heldMs > 650) {
      _held = true;
    } else {
      _clicked = true;
    }
  }
  _btnDown = nowDown;
}

int32_t RotaryInput::readDelta() {
  int32_t d = _delta;
  _delta = 0;
  return d;
}

bool RotaryInput::wasClicked() {
  bool c = _clicked;
  _clicked = false;
  return c;
}

bool RotaryInput::wasHeld() {
  bool h = _held;
  _held = false;
  return h;
}

#else

bool RotaryInput::begin() { return true; }
void RotaryInput::tick() {}
int32_t RotaryInput::readDelta() { return 0; }
bool RotaryInput::wasClicked() { return false; }

#endif
