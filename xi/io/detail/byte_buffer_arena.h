#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {
  namespace detail {

    struct byte_buffer_arena;

    struct byte_buffer_arena_deallocator : public virtual ownership::std_shared {
      virtual ~byte_buffer_arena_deallocator() = default;
      virtual void deallocate(byte_buffer_arena*) noexcept = 0;
    };

    struct byte_buffer_arena {
      usize length;
      atomic< u64 > ref_count{0};
      own< byte_buffer_arena_deallocator > deallocator;
      u8 data[0];

      byte_buffer_arena(usize l, own< byte_buffer_arena_deallocator > d)
          : length(l), deallocator(move(d)) {}

      void increment_ref_count() noexcept {
        ref_count.fetch_add(1, memory_order_relaxed);
      }
      void decrement_ref_count() noexcept {
        if (1 >= ref_count.fetch_sub(1, memory_order_relaxed)) {
          auto dealloc = deallocator;
          this->~byte_buffer_arena();
          dealloc->deallocate(this);
        }
      }
    };
    static_assert(sizeof(byte_buffer_arena) == 32, "");
  }
}
}
