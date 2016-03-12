#pragma once

#include "xi/io/byte_buffer.h"

namespace xi {
namespace io {
  struct byte_buffer_allocator : public virtual ownership::std_shared {
    virtual ~byte_buffer_allocator() = default;
    virtual own< byte_buffer > allocate(usize, usize headroom = 0) = 0;
  };
}
}
