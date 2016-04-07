#pragma once

#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"

namespace xi {
namespace io {
  // FIXME: This needs a rewrite

  struct fragment_allocator : public virtual ownership::std_shared {
    virtual ~fragment_allocator()              = default;
    virtual own< fragment > allocate(usize sz) = 0;
  };

  struct arena_fragment_allocator : public fragment_allocator {
    byte_blob _arena;
    usize _arena_size;
    usize _low_watermark;

    arena_fragment_allocator(usize arena_size, usize low_watermark = 0)
        : _arena_size(arena_size), _low_watermark(low_watermark) {
      if (!_low_watermark) {
        _low_watermark = arena_size / 100;
      }
    }

    auto make_arena() {
      auto mem       = reinterpret_cast< u8* >(malloc(_arena_size));
      auto actual_sz = malloc_usable_size(mem);
      return byte_blob(
          mem, mem ? actual_sz : 0, detail::make_simple_guard(mem));
    }

    own< fragment > allocate(usize sz) override {
      if (_arena.empty()) {
        _arena = make_arena();
      } else if (_arena.size() <= sz + _low_watermark) {
        XI_SCOPE(exit) {
          _arena = make_arena();
        };
        return make< fragment >(move(_arena));
      }
      auto blob = _arena;
      _arena = _arena.split(sz);
      return make< fragment >(move(blob));
    }
  };

  struct exact_fragment_allocator : public fragment_allocator {
    auto make_heap_byte_blob(usize sz) {
      auto mem = reinterpret_cast< u8* >(malloc(sz));
      return byte_blob(mem, mem ? sz : 0, detail::make_simple_guard(mem));
    }
    own< fragment > allocate(usize sz) override {
      return make< fragment >(make_heap_byte_blob(sz));
    }
  };

  struct simple_fragment_allocator : public fragment_allocator {
    auto make_heap_byte_blob(usize sz) {
      auto mem       = reinterpret_cast< u8* >(malloc(sz));
      auto actual_sz = malloc_usable_size(mem);
      return byte_blob(
          mem, mem ? actual_sz : 0, detail::make_simple_guard(mem));
    }
    own< fragment > allocate(usize sz) override {
      return make< fragment >(make_heap_byte_blob(sz));
    }
  };

  class basic_buffer_allocator : public buffer_allocator {
    own< fragment_allocator > _fragment_allocator;

  public:
    basic_buffer_allocator(own< fragment_allocator > alloc)
        : _fragment_allocator(move(alloc)) {
    }

    own< buffer > allocate(usize sz,
                           usize headroom = 0,
                           usize tailroom = 0) override {
      auto total_size = sz + headroom + tailroom;
      auto frag       = _fragment_allocator->allocate(total_size);
      if (headroom) {
        frag->advance(headroom);
      }
      return make< buffer >(move(frag));
    }
  };
}
}
