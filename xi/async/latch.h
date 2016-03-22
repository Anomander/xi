#pragma once

#include "xi/ext/configure.h"
#include "xi/async/future.h"

namespace xi {
namespace async {
  class latch {
    atomic< size_t > _count{0};
    promise<> _promise;

  public:
    latch(size_t c) : _count(c) {
      if (0UL == c) {
        _promise.set();
      }
    }

    void count_down(size_t amount = 1UL) {
      size_t old_count, new_count;
      old_count = _count.load();
      do {
        if (0 == old_count) {
          return;
        }
        new_count = old_count > amount ? old_count - amount : 0UL;
      } while (!_count.compare_exchange_weak(old_count, new_count));
      if (new_count == 0) {
        std::cout << "Unlocking latch" << std::endl;
        _promise.set();
      }
    }

    size_t count() const {
      return _count;
    }

    future<> await() {
      return _promise.get_future();
    }
  };
}
}
