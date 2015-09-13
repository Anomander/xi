#pragma once

#include "xi/core/Kernel.h"
#include "xi/core/ExecutorPool.h"

namespace xi {
namespace core {

  own< ExecutorPool > makeExecutorPool(mut< Kernel > kernel,  vector< unsigned >  cores = {}) {
    if (cores.empty()) {
      return make< ExecutorPool >(kernel, kernel->coreCount());
    }
    for (auto id : cores) {
      if (id >= kernel->coreCount()) {
        throw std::invalid_argument("Core id not registered: " + to_string(id));
      }
    }
    return make< ExecutorPool >(kernel, move(cores));
  }
}
}
