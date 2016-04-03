#pragma once

#include "xi/io/buffer.h"
#include "xi/io/detail/buffer_utils.h"
#include "xi/io/fragment_string.h"

namespace xi {
namespace io {

  class buffer::reader {
  protected:
    mut< buffer > _buffer;
    usize _mark = 0;

  public:
    reader(mut< buffer > b, usize m = 0) : _buffer(b), _mark(m) {
    }

    template < class T >
    bool read_value(T & value) {
      if (_mark < _buffer->size() && _buffer->size() - _mark >= sizeof(T)) {
        _buffer->read(byte_range_for_object(value), _mark);
        return true;
      }
      return false;
    }

    template < class T >
    opt< T > read_value() {
      T value;
      if (read_value(value)) {
        return some(move(value));
      }
      return none;
    }

    template < class T >
    opt< T > read_value_and_mark() {
      return read_value< T >().map([&](auto &&v) {
        _mark += sizeof(T);
        return move(v);
      });
    }

    template < class T >
    bool read_value_and_mark(T & value) {
      auto ret = read_value(value);
      if (ret) {
        _mark += sizeof(T);
      }
      return ret;
    }

    template < class T >
    opt< T > read_value_and_skip() {
      return read_value< T >().map([&](auto &&v) {
        _buffer->skip_bytes(_mark + sizeof(T));
        _mark = 0;
        return move(v);
      });
    }

    template < class T >
    bool read_value_and_skip(T & value) {
      auto ret = read_value(value);
      if (ret) {
        _buffer->skip_bytes(_mark + sizeof(T));
        _mark = 0;
      }
      return ret;
    }

    template < usize N >
    usize skip_any_not_of_and_mark(const char (&pattern)[N]) {
      return skip_any_not_of_and_mark(pattern, N - 1);
    }

    usize skip_any_not_of_and_mark(const char *pattern, usize len) {
      if (!len) {
        return _mark = _buffer->size();
      }
      return skip_any_of_and_mark((u8 *)pattern, len, [](bool b) { return b; });
    }

    template < usize N >
    usize skip_any_of_and_mark(const char (&pattern)[N]) {
      return skip_any_of_and_mark(pattern, N - 1);
    }

    usize skip_any_of_and_mark(const char *pattern, usize len) {
      if (!len) {
        return 0;
      }
      return skip_any_of_and_mark(
          (u8 *)pattern, len, [](bool b) { return !b; });
    }

    usize total_size() const;
    usize unmarked_size() const;
    usize marked_size() const;

    void mark_offset(usize);
    void discard_to_mark();
    buffer consume_mark_into_buffer();
    fragment_string consume_mark_into_string();
    template < class Pred >
    usize skip_any_of_and_mark(u8 *pattern, usize len, Pred const &);
    opt< usize > find_byte(u8 target, usize offset = 0) const;
  };

  inline buffer::reader make_reader(mut< buffer > b) {
    return buffer::reader(b);
  }

  inline usize buffer::reader::total_size() const {
    return _buffer->size();
  }

  inline usize buffer::reader::unmarked_size() const {
    return _buffer->size() - _mark;
  }

  inline usize buffer::reader::marked_size() const {
    return _mark;
  }

  inline void buffer::reader::mark_offset(usize offset){
    assert(make_signed_t<usize>(_mark + offset) > 0);
    _mark += min(offset, unmarked_size());
  }

  inline void buffer::reader::discard_to_mark() {
    XI_SCOPE(success) {
      _mark = 0;
    };
    return _buffer->skip_bytes(_mark);
  }

  inline buffer buffer::reader::consume_mark_into_buffer() {
    XI_SCOPE(success) {
      _mark = 0;
    };
    return _buffer->split(_mark);
  }

  inline fragment_string buffer::reader::consume_mark_into_string() {
    auto buf = _buffer->split(_mark);
    _mark    = 0;
    return fragment_string{move(buf._fragments), buf._size};
  }

  template < class Pred >
  usize buffer::reader::skip_any_of_and_mark(u8 *pattern,
                                             usize len,
                                             Pred const &predicate) {
    usize ret   = 0;
    auto end    = _buffer->_fragments.end();
    auto offset = _mark;
    auto it     = skip_offset(_buffer->_fragments.begin(), end, edit(offset));

    while (end != it) {
      auto ch     = it->data_range().data() + offset;
      auto ch_end = it->data_range().data() + it->size();
      while (ch != ch_end) {
        if (predicate(::memchr(pattern, *(ch++), len))) {
          goto done;
        }
        ++ret;
      }
      offset = 0;
      ++it;
    }
  done:
    _mark += ret;
    return ret;
  }

  opt< usize > buffer::reader::find_byte(u8 target, usize offset) const {
    if (offset >= _buffer->size()) {
      return none;
    }

    auto end = _buffer->_fragments.end();
    auto it  = skip_offset(_buffer->_fragments.begin(), end, edit(offset));

    void *found = nullptr;
    usize size  = 0u;
    do {
      found = ::memchr(it->data() + offset, target, it->size() - offset);
      if (found) {
        size += ((u8 *)found - it->data()) - offset;
      } else {
        size += it->size() - offset;
        offset = 0;
      }
      ++it;
    } while (nullptr == found && end != it);
    return found ? some(size) : none;
  }
}
}
