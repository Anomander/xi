#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {
  namespace detail {

    struct buffer_arena;

    struct buffer_arena_deallocator : public virtual ownership::std_shared {
      virtual ~buffer_arena_deallocator() = default;
      virtual void deallocate(buffer_arena*) noexcept = 0;
    };

    struct buffer_arena {
      usize length;
      atomic< u64 > ref_count{0};
      own< buffer_arena_deallocator > deallocator;
      u8 data[0];

      buffer_arena(usize l, own< buffer_arena_deallocator > d)
          : length(l), deallocator(move(d)) {}

      void increment_ref_count() noexcept {
        ref_count.fetch_add(1, memory_order_relaxed);
      }
      void decrement_ref_count() noexcept {
        if (1 >= ref_count.fetch_sub(1, memory_order_relaxed)) {
          auto dealloc = deallocator;
          this->~buffer_arena();
          dealloc->deallocate(this);
        }
      }
    };
    static_assert(sizeof(buffer_arena) == 32, "");
  }
}
}
