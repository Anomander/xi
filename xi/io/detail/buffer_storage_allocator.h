#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {
  namespace detail {

    struct buffer_storage_allocator : public virtual ownership::std_shared {
      virtual ~buffer_storage_allocator() = default;
      virtual void* allocate(usize)       = 0;
      virtual void free(void*)            = 0;
    };
  }
}
}
