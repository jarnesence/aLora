#pragma once
#include <stdint.h>
#include <stddef.h>

struct ChatMsg {
  uint32_t ts;
  uint16_t src;
  bool outgoing;
  char text[96];
};

class ChatLog {
public:
  static constexpr size_t MAX = 30;

  void add(uint16_t src, bool outgoing, const char* text, uint32_t ts);
  size_t size() const { return _count; }
  // idx=0 oldest ... idx=size-1 newest
  const ChatMsg& at(size_t idx) const;

private:
  ChatMsg _buf[MAX]{};
  size_t _head = 0;   // next write position
  size_t _count = 0;  // number of valid messages
};
