#pragma once

#include "xi/io/buf/arena_base.h"

namespace xi {
namespace io {

  class buffer : public ownership::unique {
    own< arena_base > _arena;
    size_t _offset, _size;
    mut< buffer > _next = this;
    mut< buffer > _prev = this;

  public:
    class view;
    class chain;

  public:
    buffer(own< arena_base > arena, size_t offset, size_t len)
        : _arena(move(arena)), _offset(offset), _size(len) {}

  public:
    uint8_t* begin();
    uint8_t* end();
    uint8_t const* begin() const;
    uint8_t const* end() const;

    size_t size() const;

    view make_view(size_t offset, size_t length) const;

  public:
    void operator delete(void*) {}
  };

  inline uint8_t* buffer::begin() { return _arena->data() + _offset; }

  inline uint8_t* buffer::end() { return begin() + _size; }

  inline uint8_t const* buffer::begin() const {
    return const_cast< buffer* >(this)->begin();
  }

  inline uint8_t const* buffer::end() const {
    return const_cast< buffer* >(this)->end();
  }

  inline size_t buffer::size() const { return _size; }
}
}
