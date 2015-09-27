#pragma once

#include "xi/core/kernel.h"
#include "xi/core/executor_pool.h"

namespace xi {
namespace core {

  own< executor_pool > make_executor_pool(mut< kernel > kernel,
                                          vector< unsigned > cores = {}) {
    if (cores.empty()) {
      return make< executor_pool >(kernel, kernel->core_count());
    }
    for (auto id : cores) {
      if (id >= kernel->core_count()) {
        throw std::invalid_argument("Core id not registered: " + to_string(id));
      }
    }
    return make< executor_pool >(kernel, move(cores));
  }
}
}
