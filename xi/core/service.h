#pragma once

#include "xi/core/executor_pool.h"

namespace xi {
namespace core {
  template < class I >
  class service {
  public:
    service(own< executor_pool > pool) : _pool(move(pool)) {
    }

  public:
    own< executor_pool > _pool;
  };
}
}
