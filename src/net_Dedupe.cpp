#include "net/Dedupe.h"

void DedupeCache::clear() {
  for (auto &x : _e) { x.src = 0; x.msgId = 0; }
  _idx = 0;
}

bool DedupeCache::seen(uint16_t src, uint32_t msgId) const {
  for (auto &x : _e) {
    if (x.src == src && x.msgId == msgId) return true;
  }
  return false;
}

void DedupeCache::remember(uint16_t src, uint32_t msgId) {
  _e[_idx].src = src;
  _e[_idx].msgId = msgId;
  _idx = (uint8_t)((_idx + 1) % (sizeof(_e)/sizeof(_e[0])));
}
