#pragma once

#include "xi/deps/configure.h"

namespace xi {

class SpinLock {
  enum LockState { Locked, Unlocked };

  atomic< LockState > _state;

public:
  SpinLock() : _state(Unlocked) {
  }

  void lock() {
    for (size_t attempt = 0; _state.exchange(Locked, memory_order_acquire) == Locked; ++attempt) {
      if (attempt < 4) { /* do nothing */
      } else if (attempt < 16) {
        __asm__ __volatile__("rep; nop" : : : "memory");
      } else {
        deps::this_thread::yield();
      }
    }
  }
  void unlock() {
    _state.store(Unlocked, memory_order_release);
  }
};

} // namespace xi
