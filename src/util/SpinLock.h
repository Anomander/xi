#pragma once

#include "ext/Configure.h"

namespace xi {

class SpinLock {
  enum LockState : uint8_t { Locked, Unlocked };

  atomic< LockState > _state;

public:
  SpinLock() : _state(Unlocked) {}

  void lock() {
    for (size_t attempt = 0; _state.exchange(Locked, memory_order_acquire) == Locked; ++attempt) {
      if (attempt < 4) { /* do nothing */
      } else if (attempt < 16) {
        __asm__ __volatile__("rep; nop" : : : "memory");
      } else {
        ::sched_yield();
      }
    }
    assert(_state.load(memory_order_relaxed) == Locked);
  }
  void unlock() { _state.store(Unlocked, memory_order_release); }
};

} // namespace xi
