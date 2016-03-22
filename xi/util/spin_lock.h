#pragma once

#include "xi/ext/configure.h"

namespace xi {

class spin_lock {
  enum lock_state : uint8_t { locked, unlocked };

  atomic< lock_state > _state;

public:
  spin_lock() : _state(unlocked) {
  }
  spin_lock(spin_lock &&other)
      : _state(other._state.load(memory_order_acquire)) {
    other.unlock();
  }

  void lock() {
    for (size_t attempt = 0;
         _state.exchange(locked, memory_order_acquire) == locked;
         ++attempt) {
      if (attempt >= 8) {
        /// memory barrier. there's no sense in yielding as
        /// threads don't compete, but hint the PU about busy-wait.
        __asm__ volatile("pause" ::: "memory");
      }
    }
    assert(_state.load(memory_order_relaxed) == locked);
  }
  void unlock() {
    _state.store(unlocked, memory_order_release);
  }
};

} // namespace xi
