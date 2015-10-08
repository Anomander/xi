#pragma once

#include "xi/io/buf/buffer.h"
#include "xi/io/buf/buffer_allocator.h"

namespace xi {
namespace io {

  class buffer::chain {
    own< buffer > _head;
    size_t _size = 0;

    struct helper;

  public:
    class iterator;
    class cursor;

  public:
    chain() = default;
    chain(own< buffer > head);
    ~chain() noexcept;

    chain(chain&&) = default;
    chain& operator=(chain&&) = default;
    chain(chain const&) = delete;
    chain& operator=(chain const&) = delete;

    iterator begin();
    iterator end();

    void push_back(own< buffer >);

    void push_back(chain&&);
    void push_front(chain&&);

    own< buffer > pop(iterator);
    chain pop(iterator, iterator);

    size_t size() const { return _size; }

    cursor make_cursor(size_t offset = 0);

    iterator gather(iterator, iterator, mut< buffer_allocator >);
    buffer::view make_view(iterator, iterator, mut< buffer_allocator >);

  private:
    mut< buffer > release_head();
  };
}
}
