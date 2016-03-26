#pragma once

#include "xi/ext/configure.h"
#include "xi/core/async.h"
#include "xi/core/shard.h"

namespace xi {
namespace core {

  template < class T, class F >
  void defer(xi::core::async<T> * a, F &&f) {
    if (!is_valid(a->shard())) {
      throw std::logic_error("Invalid async object state.");
    }
    a->shard()->post(forward< F >(f));
  }
}
}
