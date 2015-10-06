#pragma once

#include "xi/core/kernel.h"
#include "xi/core/executor_pool.h"

namespace xi {
namespace async {

  class resolve_iterator {
    
  };

  class resolver_service : public virtual ownership::std_shared {
  public:
    void start(mut< core::executor_pool > pool) {
    }
    void stop() {
    }
    resolve_iterator resolve(string addr) { return {}; }
  };
}
}
