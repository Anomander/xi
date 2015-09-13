#pragma once

#include "xi/ext/Configure.h"

namespace xi {

class SpinLock {
  enum LockState : uint8_t { Locked, Unlocked };

  atomic< LockState > _state;

public:
  SpinLock() : _state(Unlocked) {}
  SpinLock(SpinLock && other) : _state(other._state.load(memory_order_acquire)) {
    other.unlock();
  }

  void lock() {
    for (size_t attempt = 0; _state.exchange(Locked, memory_order_acquire) == Locked; ++attempt) {
      if (attempt >= 8) {
        /// Memory barrier. There's no sense in yielding as
        /// threads don't compete, but hint the PU about busy-wait.
        __asm__ volatile("pause" ::: "memory");
      }
    }
    assert(_state.load(memory_order_relaxed) == Locked);
  }
  void unlock() { _state.store(Unlocked, memory_order_release); }
};

} // namespace xi
