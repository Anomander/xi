#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {
  namespace detail {
    struct buffer_arena;

    struct buffer_arena_deallocator : public virtual ownership::std_shared {
      virtual ~buffer_arena_deallocator()             = default;
      virtual void deallocate(buffer_arena*) noexcept = 0;
    };

    struct buffer_arena {
      usize length;
      usize start = 0;
      u64 ref_count = 1;
      mut< buffer_arena_deallocator > deallocator;
      u8 data[0];

      buffer_arena(usize l, mut< buffer_arena_deallocator > d)
          : length(l), deallocator(move(d)) {
      }

      void increment_ref_count() noexcept {
        ++ref_count;
      }
      void decrement_ref_count() noexcept {
        if (0 == --ref_count) {
          auto dealloc = deallocator;
          this->~buffer_arena();
          dealloc->deallocate(this);
        }
      }

      usize free_size() {
        return length - start;
      }

      void consume(usize sz) {
        start += sz;
      }

      u8* allocate(usize sz) {
        if ( sz > free_size() ) {
          return nullptr;
        }
        return data + start;
      }
    };
    static_assert(sizeof(buffer_arena) == 32, "");
  }
}
}
