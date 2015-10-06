#pragma once
#include "xi/io/buf/buf_base.h"
#include "xi/io/buf/mutable_byte_range.h"
#include "xi/io/buf/buffer.h"

namespace xi {
namespace io {
  namespace detail {
    constexpr size_t aligned_size(size_t sz, size_t align = alignof(void*)) {
      return (sz + align - 1) & ~(align - 1);
    }
  }

  class buf : public buf_base {
  public:
    using buf_base::buf_base;
    opt< mutable_byte_range > allocate_range(size_t length) {
      if (remaining() < length) { return none; }
      auto old_consumed = consumed();
      consume(length);
      return some(mutable_byte_range{share(this), old_consumed, length});
    }
    opt< unique_ptr< buffer > > allocate_buffer(size_t sz) {
      auto actual_size = sz + detail::aligned_size(sizeof(buffer));
      if (remaining() < actual_size) { return none; }
      auto old_consumed = consumed();
      consume(actual_size);
      return some(unique_ptr< buffer >(new (data() + old_consumed) buffer(
          share(this), old_consumed + sizeof(buffer), sz)));
    }
  };
}
}
