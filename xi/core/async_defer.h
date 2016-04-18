#pragma once

#include "xi/ext/configure.h"
#include "xi/core/async.h"
#include "xi/core/shard.h"

namespace xi {
namespace core {

  template < class T, class F >
  void defer(async< T > *a, F &&f) {
    if (!a || !is_valid(a->shard())) {
      throw std::logic_error("Invalid async object state.");
    }
    a->shard()->post(
        make_task([ context = a->async_context(), f = move(f) ]() mutable {
          if (auto lock = context.lock()) {
            f();
          }
        }));
  }
}
}
