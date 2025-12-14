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

private:
  int32_t _delta = 0;
  bool _clicked = false;
};
