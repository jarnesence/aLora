#include "net/Dedupe.h"

static inline size_t clampCap(size_t v) {
  if (v == 0) return 1;
  if (v > DedupeCache::kMax) return DedupeCache::kMax;
  return v;
}

DedupeCache::DedupeCache(size_t capacity) : _cap(clampCap(capacity)), _idx(0) {
  for (size_t i = 0; i < kMax; i++) {
    _e[i].src = 0;
    _e[i].msgId = 0;
  }
}

bool DedupeCache::seen(uint16_t src, uint32_t msgId) const {
  for (size_t i = 0; i < _cap; i++) {
    if (_e[i].src == src && _e[i].msgId == msgId) return true;
  }
  return false;
}

void DedupeCache::remember(uint16_t src, uint32_t msgId) {
  _e[_idx].src = src;
  _e[_idx].msgId = msgId;

  _idx++;
  if (_idx >= _cap) _idx = 0;
}
