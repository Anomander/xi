#pragma once

#include "xi/ext/configure.h"
#include "xi/core/runtime.h"
#include "xi/core/resumable.h"

namespace xi {
  namespace core {
    inline void yield() {
      auto r = runtime.local_worker().current_resumable();
      if (r) {
        r->yield(resumable::resume_later);
      }
    }
  }
}
