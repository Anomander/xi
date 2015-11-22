#pragma once

#include "xi/io/buf/buffer.h"
#include "xi/io/buf/buffer_allocator.h"

namespace xi {
namespace io {

  class buffer::chain {
    buffer _root;
    mut< buffer > _tail;
    size_t _size = 0;

    struct helper;
    struct $;

  public:
    class iterator;
    class cursor;
    class iovec_adapter;

  public:
    chain();
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

    size_t size() const;

    cursor make_cursor(size_t offset = 0);

    iterator gather(iterator, iterator, mut< buffer_allocator >);
    buffer::view make_view(iterator, iterator, mut< buffer_allocator >);

    size_t consume_head(size_t n);
    size_t consume_tail(size_t n);
    size_t append_head(size_t n);
    size_t append_tail(size_t n);

    size_t headroom() const;
    size_t tailroom() const;
    bool has_tailroom() const;

    void print();
  private:
    mut< buffer > release_head();
    mut< buffer > head() const;
    mut< buffer > tail() const;
  };

  inline size_t buffer::chain::size() const { return _size; }
  inline bool buffer::chain::has_tailroom() const { return _tail->tailroom() > 0; }
}
}
