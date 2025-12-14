#pragma once
#include <stdint.h>
#include <stddef.h>

// Simple fixed-size deduplication cache for (src,msgId).
// No heap allocations; safe for small MCUs.
class DedupeCache {
public:
  static const size_t kMax = 64;

  explicit DedupeCache(size_t capacity = kMax);

  bool seen(uint16_t src, uint32_t msgId) const;
  void remember(uint16_t src, uint32_t msgId);

private:
  struct Entry { uint16_t src; uint32_t msgId; };

  size_t _cap;
  size_t _idx;
  Entry _e[kMax];
};
