#pragma once

#include "xi/io/buffer.h"

namespace xi {
namespace io {
  struct buffer_allocator : public virtual ownership::std_shared {
    virtual ~buffer_allocator() = default;
    virtual buffer allocate(usize, usize headroom = 0, usize tailroom = 0) = 0;
  };
}
}
