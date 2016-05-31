#pragma once

#include "xi/ext/configure.h"
#include "xi/core/resumable.h"

namespace xi {
namespace core {
  namespace v2 {

    class resumable_builder : public virtual ownership::unique {
    public:
      virtual ~resumable_builder()     = default;
      virtual own< resumable > build() = 0;
    };
  }
}
}
