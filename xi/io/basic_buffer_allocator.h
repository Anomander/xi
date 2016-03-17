#pragma once

#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/detail/buffer_arena.h"
#include "xi/io/detail/buffer_storage_allocator.h"

namespace xi {
namespace io {
  namespace detail {
    constexpr size_t aligned_size(size_t sz, size_t align = alignof(void*)) {
      return (sz + align - 1) & ~(align - 1);
    }
  }
  // FIXME: This needs a rewrite

  template < class SA >
  class buffer_arena_allocator : public detail::buffer_arena_deallocator {
    own< SA > _storage_alloc = make< SA >();

  public:
    buffer_arena_allocator() = default;
    buffer_arena_allocator(own< SA > storage_alloc)
        : _storage_alloc(move(storage_alloc)) {
    }

    detail::buffer_arena* allocate_arena(usize sz) {
      auto size = sz + detail::aligned_size(sizeof(detail::buffer_arena));
      return new (_storage_alloc->allocate(size))
          detail::buffer_arena{sz, share(this)};
    };

    void deallocate(detail::buffer_arena* c) noexcept override {
      _storage_alloc->free(c);
    }
  };

  template < class SA >
  class basic_buffer_allocator : public buffer_allocator,
                                 public buffer_arena_allocator< SA > {
  public:
    using buffer_arena_allocator< SA >::buffer_arena_allocator;

    buffer allocate(usize sz, usize headroom = 0, usize tailroom = 0) override {
      auto total_size = sz + headroom + tailroom;
      auto c = this->allocate_arena(total_size);
      XI_SCOPE(fail) {
        this->deallocate(c);
      };
      auto frag = own< fragment >{new fragment(c, 0, total_size)};
      if (headroom) {
        frag->advance(headroom);
      }
      return {move(frag)};
    }
  };
}
}
