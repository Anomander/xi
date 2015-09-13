#pragma once

#include "xi/core/ExecutorPool.h"

namespace xi {
namespace core {
  template < class I >
  class Service {
  public:
    Service(own< ExecutorPool > pool) : _pool(move(pool)) {}

  public:
    own< ExecutorPool > _pool;
  };
}
}
