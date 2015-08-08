#pragma once

#include "ext/Configure.h"
#include "async/Future.h"

namespace xi {
namespace async {
  class Latch {
    atomic< size_t > _count{0};
    Promise<> _promise;

  public:
    Latch(size_t c) : _count(c) {
      if (0UL == c) {
        _promise.setValue();
      }
    }

    void countDown(size_t amount = 1UL) {
      auto count = _count.load(memory_order_acquire);
      if (0UL == count ) {
        return;
      }
      auto newValue = amount >= count ? 0UL : count - amount;
      if (_count.compare_exchange_strong(count, newValue, memory_order_release, memory_order_acquire) &&
          0 == newValue) {
        _promise.setValue();
      }
    }

    Future<> await() { return _promise.getFuture(); }
  };
}
}
