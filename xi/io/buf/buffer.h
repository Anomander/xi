#pragma once
#include "xi/io/buf/buf_base.h"

namespace xi {
namespace io {

  class buffer : public virtual ownership::unique {
    own< buf_base > _arena;
    uint32_t _offset = 0;
    uint32_t _length = 0;
    buffer* _next = this;
    buffer* _prev = this;

  public:
    class view;

  public:
    buffer(own< buf_base > arena, size_t offset, size_t length)
        : _arena(move(arena)), _offset(offset), _length(length) {}

    ~buffer() {
      auto n = _next;
      while (n != this) {
        auto p = n;
        n = n->_next;
        delete p;
      }
    }

    uint8_t* writable_bytes() { return _arena->data() + _offset; }
    uint8_t* begin() { return writable_bytes(); }
    uint8_t* end() { return begin() + _length; }

    const uint8_t* begin() const { return _arena->data() + _offset; }
    const uint8_t* end() const { return begin() + _length; }
    size_t length() const { return _length; }
    bool is_empty() const { return _length == 0; }

    uint32_t consume_head(uint32_t n) {
      auto consumed = min(n, _length);
      _offset += consumed;
      _length -= consumed;
      return n - consumed;
    }
    uint32_t consume_tail(uint32_t n) {
      auto consumed = min(n, _length);
      _length -= consumed;
      return n - consumed;
    }

  public:
    void operator delete(void* p) {}

  public:
    void push_back(unique_ptr< buffer > b);
    void push_front(unique_ptr< buffer > b);
    unique_ptr< buffer > unlink();
    unique_ptr< buffer > pop();

    mut< buffer > next() { return _next; }
    mut< buffer > prev() { return _prev; }

  public:
    view make_view(uint32_t offset = 0, uint32_t length = -1);
  };

  class buffer::view final {
    own< buf_base > _arena;
    uint32_t _offset = 0;
    uint32_t _length = 0;

    friend class buffer;
    view(ref< buffer > buf, uint32_t off, uint32_t length)
        : _arena(share(buf._arena)), _offset(off), _length(length) {}

  public:
    /* implicit */ view(ref< buffer > buf)
        : _arena(share(buf._arena))
        , _offset(buf._offset)
        , _length(buf._length) {}

    const uint8_t* readable_bytes() const { return _arena->data() + _offset; }
    const uint8_t* begin() const { return readable_bytes(); }
    const uint8_t* end() const { return begin() + _length; }
    size_t length() const { return _length; }
  };

  auto buffer::make_view(uint32_t offset, uint32_t length) -> view {
    auto actual_offset = min(offset, _length);
    return view(*this, actual_offset, max(_length - actual_offset, length));
  }

  void buffer::push_back(unique_ptr< buffer > b) {
    auto n = _next;
    _next = b.release();
    _next->_next = n;
    _next->_prev = this;
    n->_prev = _next;
  }

  void buffer::push_front(unique_ptr< buffer > b) { _prev->push_back(move(b)); }

  unique_ptr< buffer > buffer::unlink() {
    _prev->_next = _next;
    _next->_prev = _prev;
    _next = _prev = this;
    return unique_ptr< buffer >(this);
  }

  unique_ptr< buffer > buffer::pop() {
    auto old_next = _next;
    _prev->_next = _next;
    _next->_prev = _prev;
    _next = _prev = this;
    return unique_ptr< buffer >(old_next ? (buffer*)nullptr : old_next);
  }
}
}
