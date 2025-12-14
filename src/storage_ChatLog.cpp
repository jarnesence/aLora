#include <cstring>
#include "storage/ChatLog.h"

void ChatLog::add(uint16_t src, bool outgoing, uint32_t msgId, const char* text, uint32_t ts) {
  ChatMsg& m = _buf[_head];
  m.ts = ts;
  m.src = src;
  m.outgoing = outgoing;
  m.delivered = !outgoing;  // inbound is implicitly delivered
  m.msgId = msgId;
  std::strncpy(m.text, text ? text : "", sizeof(m.text)-1);
  m.text[sizeof(m.text)-1] = '\0';

  _head = (_head + 1) % MAX;
  if (_count < MAX) _count++;
}

const ChatMsg& ChatLog::at(size_t idx) const {
  // map idx (oldest..newest) into circular buffer
  size_t start = (_head + MAX - _count) % MAX;
  size_t pos = (start + idx) % MAX;
  return _buf[pos];
}

bool ChatLog::markDelivered(uint32_t msgId) {
  size_t start = (_head + MAX - _count) % MAX;
  for (size_t i = 0; i < _count; i++) {
    size_t pos = (start + i) % MAX;
    ChatMsg& m = _buf[pos];
    if (!m.outgoing) continue;
    if (m.msgId != msgId) continue;
    m.delivered = true;
    return true;
  }
  return false;
}
