#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {
  namespace detail {

    struct byte_buffer_storage_allocator : public virtual ownership::std_shared {
      virtual ~byte_buffer_storage_allocator() = default;
      virtual void* allocate(usize) = 0;
      virtual void free(void*) = 0;
    };

  }
}
}
