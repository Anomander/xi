#pragma once
#include "xi/io/buf/buffer.h"

namespace xi {
namespace io {

  class buffer_queue final {
    own< buffer > _head;
    mut< buffer > _tail = edit(_head);
    size_t _total_length = 0;

  public:
    class cursor;

  public:
    void push_back(own< buffer > b) {
      if (!is_valid(_head)) { _head = move(b); } else {
        _tail->push_back(move(b));
      }
      _tail = _head->prev();
      _total_length += b->length();
    }
    unique_ptr< buffer > pop() {
      auto head = move(_head);
      _head = head->pop();
      _total_length -= head->length();
      return move(head);
    }
    uint32_t consume_head(uint32_t n) {
      auto remaining = n;
      while (!is_empty() && remaining > 0) {
        remaining = _head->consume_head(remaining);
        if (_head->is_empty()) { pop(); }
      }
      _total_length -= n;
      return remaining;
    }
    uint32_t consume_tail(uint32_t n) {
      auto remaining = n;
      while (!is_empty() && remaining > 0) {
        remaining = _tail->consume_tail(remaining);
        if (_tail->is_empty()) { _tail->unlink(); }
      }
      _total_length -= n;
      return remaining;
    }
    bool is_empty() const { return !is_valid(_head); }
    size_t length() const { return _total_length; }
    mut< buffer > head() { return edit(_head); }
  };

  class buffer_queue::cursor {
    mut< buffer_queue > _queue;
    mut< buffer > _original_buf;
    mut< buffer > _current_buf;
    size_t _offset = 0;
    size_t _position = 0;

  public:
    cursor(mut< buffer_queue > q)
        : _queue(q)
        , _original_buf(_queue->head())
        , _current_buf(_queue->head()) {}

    void read(uint8_t* data, size_t len) {
      if (len + _offset > _queue->length()) {
        throw std::out_of_range("Not enough bytes");
      }
      while (len > 0) {
        auto actual_len = min(len, _current_buf->length() - _offset);
        memcpy(data, _current_buf->writable_bytes() + _offset, actual_len);
        if (_current_buf->length() == _offset + actual_len) {
          _offset = 0;
          _position += _current_buf->length();
          _current_buf = _current_buf->next();
        } else {
          _offset += actual_len;
          _position += actual_len;
        }
        len -= actual_len;
      }
    }

    template < class T, XI_REQUIRE_DECL(is_arithmetic< T >)> T read() {
      T t;
      read((uint8_t*)address_of(t), sizeof(t));
      return t;
    }

    void skip(size_t n) { _advance_offset(n); }

    size_t position() const { return _position; }

  private:
    void _advance_offset(size_t n) {
      do {
        auto to_go = min(_current_buf->length() - _offset, n);
        if (XI_UNLIKELY(0 == to_go)) { return; }
        _position += to_go;
        n -= to_go;
        if (XI_LIKELY(_current_buf->next() != _original_buf)) {
          _offset += to_go;
          _current_buf = _current_buf->next();
          continue;
        } else { _offset = _current_buf->length(); }
      } while (false);
    }
  };
}
}
