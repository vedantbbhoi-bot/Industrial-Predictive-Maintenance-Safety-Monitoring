#pragma once
#include <stdint.h>
// Time-based debounce. Feed it the raw pin state (true = pressed/active) on every loop.
class Debounce {
public:
  explicit Debounce(uint32_t ms) : ms_(ms) {}
  bool update(uint32_t now, bool raw) {
    bool prev = stable_;
    if (raw != last_) { last_ = raw; t_ = now; }
    else if (now - t_ >= ms_) stable_ = raw;
    rose_ = stable_ && !prev;
    return stable_;
  }
  bool rose() const { return rose_; }   // true only on the call where it became active
private:
  uint32_t ms_, t_ = 0; bool last_ = false, stable_ = false, rose_ = false;
};
