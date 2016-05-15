#pragma once

#include "xi/ext/configure.h"
#include "xi/core/runtime.h"
#include "xi/core/policy/worker_isolation.h"

namespace xi {
  namespace core {
    template<class F, XI_REQUIRE(is_base_of<resumable, F>)>
    void spawn() {
      
    }
  }
}
