#pragma once
#include <stdint.h>

class RotaryInput {
public:
  bool begin();
  void tick();

  // Returns delta since last call (can be negative).
  int32_t readDelta();

  // Button edge events
  bool wasClicked();
  bool wasHeld();

private:
  int32_t _delta = 0;
  bool _clicked = false;
  bool _held = false;
  bool _btnDown = false;
  uint32_t _downSinceMs = 0;
};
