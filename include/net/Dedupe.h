#pragma once
#include <stdint.h>

class DedupeCache {
public:
  void clear();
  bool seen(uint16_t src, uint32_t msgId) const;
  void remember(uint16_t src, uint32_t msgId);

private:
  struct Entry { uint16_t src=0; uint32_t msgId=0; };
  Entry _e[24];
  uint8_t _idx = 0;
};
