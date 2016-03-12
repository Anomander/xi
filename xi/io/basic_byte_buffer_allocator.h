#pragma once

#include "xi/io/byte_buffer_allocator.h"
#include "xi/io/detail/byte_buffer_arena.h"
#include "xi/io/detail/byte_buffer_storage_allocator.h"

namespace xi {
namespace io {
  namespace detail {
    constexpr size_t aligned_size(size_t sz, size_t align = alignof(void*)) {
      return (sz + align - 1) & ~(align - 1);
    }
  }
  template < class SA >
  class basic_byte_buffer_allocator
      : public byte_buffer_allocator,
        public detail::byte_buffer_arena_deallocator {
    own< SA > _storage_alloc = make< SA >();

  public:
    basic_byte_buffer_allocator() = default;
    basic_byte_buffer_allocator(own< SA > storage_alloc)
        : _storage_alloc(move(storage_alloc)) {}

    detail::byte_buffer_arena* allocate_byte_buffer_arena(usize sz) {
      auto size = sz + detail::aligned_size(sizeof(detail::byte_buffer_arena));
      return new (_storage_alloc->allocate(size))
          detail::byte_buffer_arena{sz, share(this)};
    };

    own< byte_buffer > allocate(usize sz, usize headroom = 0) override {
      auto total_size = sz + headroom;
      auto c = allocate_byte_buffer_arena(total_size);
      XI_SCOPE(fail) { _storage_alloc->free(c); };
      auto ret = own< byte_buffer >{new byte_buffer(c, 0, total_size)};
      ret->advance(headroom);
      return move(ret);
    }

    void deallocate(detail::byte_buffer_arena* c) noexcept override {
      _storage_alloc->free(c);
    }
  };
}
}
