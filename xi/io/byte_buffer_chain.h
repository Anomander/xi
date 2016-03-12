#pragma once

#include "xi/io/byte_buffer.h"

namespace xi {
namespace io {

  class byte_buffer_allocator;

  class byte_buffer::chain {
    own< byte_buffer > _head = nullptr;
    usize _length = 0;
    usize _size = 0;

    struct util;

  private:
    chain(own< byte_buffer >, usize, usize);

  public:
    chain() = default;
    chain(own< byte_buffer >);
    XI_ONLY_MOVABLE(chain);

    mut< byte_buffer > head() const;
    mut< byte_buffer > tail() const;

    void push_front(own< byte_buffer >, bool pack = false);
    void push_back(own< byte_buffer >, bool pack = false);
    void push_front(chain, bool pack = false);
    void push_back(chain, bool pack = false);

    own< byte_buffer > pop_front();
    own< byte_buffer > pop_back();

    bool empty() const;
    usize length() const;
    usize compute_writable_size() const;
    usize size() const;

    chain split(usize = -1);
    usize coalesce(mut< byte_buffer_allocator > alloc, usize = -1);

    void trim_start(usize);
    void trim_end(usize);

    usize read(byte_range);
    usize write(const byte_range);

  private:
    own< byte_buffer > pop(mut< byte_buffer >);
  };

  inline mut< byte_buffer > byte_buffer::chain::head() const {
    return _head.get();
  }
  inline mut< byte_buffer > byte_buffer::chain::tail() const {
    return _head ? _head->_prev : nullptr;
  }
  inline usize byte_buffer::chain::size() const { return _size; }
}
}
