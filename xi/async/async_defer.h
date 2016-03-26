#pragma once

#include "xi/ext/configure.h"
#include "xi/async/async.h"
#include "xi/core/shard.h"

namespace xi {
namespace async {

  template < class T, class F >
  void defer(xi::async::async<T> * a, F &&f) {
    if (!is_valid(a->shard())) {
      throw std::logic_error("Invalid async object state.");
    }
    a->shard()->post(forward< F >(f));
  }
}
}
