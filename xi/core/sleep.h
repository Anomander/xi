#pragma once

#include "xi/ext/configure.h"
#include "xi/core/runtime.h"
#include "xi/core/resumable.h"

namespace xi {
namespace core {
  namespace v2 {
    inline void sleep(nanoseconds ns) {
      runtime.sleep_current_resumable(ns);
    }
  }
  inline void sleep(nanoseconds ns) {
    auto r = runtime.local_worker().current_resumable();
    if (r) {
      r->sleep_for(ns);
    }
  }
}
}
