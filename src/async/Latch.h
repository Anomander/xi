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
      size_t oldCount, newCount;
      oldCount = _count.load();
      do {
        if (0 == oldCount) {
          return;
        }
        newCount = oldCount > amount ? oldCount - amount : 0UL;
      } while (!_count.compare_exchange_weak(oldCount, newCount));
      if (newCount == 0) {
        std::cout<<"Unlocking latch"<<std::endl;
        _promise.setValue();
      }
    }

    size_t count() const { return _count; }

    Future<> await() { return _promise.getFuture(); }
  };
}
}
