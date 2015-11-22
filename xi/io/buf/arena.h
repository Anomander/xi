#pragma once
#include "xi/io/buf/arena_base.h"
#include "xi/io/buf/buffer.h"

namespace xi {
namespace io {
  namespace detail {
    constexpr size_t aligned_size(size_t sz, size_t align = alignof(void*)) {
      return (sz + align - 1) & ~(align - 1);
    }
  }

  class arena : public arena_base {
  public:
    using arena_base::arena_base;
    opt< unique_ptr< buffer > > allocate_buffer(size_t sz) {
      auto actual_size = sz + detail::aligned_size(sizeof(buffer));
      if (remaining() < actual_size) { return none; }
      auto old_consumed = consumed();
      consume(actual_size);
      return some(unique_ptr< buffer >(new (data() + old_consumed) buffer(
          share(this), old_consumed + detail::aligned_size(sizeof(buffer)),
          sz)));
    }
  };
}
}
