#pragma once

#include "xi/io/buffer.h"
#include "xi/io/detail/buffer_utils.h"

namespace xi {
namespace io {

  class buffer::reader {
  protected:
    mut< buffer > _buffer;
    bool _consume = false;

  public:
    reader(mut< buffer > b, bool consume) : _buffer(b), _consume(consume) {
    }

    template < class T >
    opt< T > read() {
      auto val = peek< T >();
      return _consume ? val.map([this](auto t) mutable {
        _buffer->skip_bytes(sizeof(T));
        return move(t);
      })
                      : move(val);
    }

    template < class T >
    opt< T > peek() const {
      if (_buffer->size() >= sizeof(T)) {
        T value;
        _buffer->read(byte_range_for_object(value));
        return some(value);
      }
      return none;
    }

    template < usize N >
    usize skip_any_of(const char (&pattern)[N]) {
      return skip_any_of((u8 *)pattern, N - 1);
    }

    usize peek(byte_range);
    usize read_string(mut< string > s, usize = -1);
    usize skip_any_of(u8 *pattern, usize len);
    opt< usize > find_byte(u8 target, usize offset = 0) const;
  };

  inline buffer::reader make_consuming_reader(mut< buffer > b) {
    return buffer::reader(b, true);
  }

  inline buffer::reader make_reader(mut< buffer > b) {
    return buffer::reader(b, false);
  }


  inline usize buffer::reader::peek(byte_range r) {
    if (!r.empty() && _buffer->size() >= r.size()) {
      return _buffer->read(r);
    }
    return 0;
  }

  inline usize buffer::reader::read_string(mut< string > s, usize len) {
    if (0 == _buffer->size()) {
      return 0;
    }
    auto it   = _buffer->_fragments.begin();
    auto end  = _buffer->_fragments.end();
    auto size = len;

    while (!it->size()) {
      ++it;
    }

    auto consume_begin = it;
    while (len && it != end) {
      auto r = it->data_range();
      if (XI_UNLIKELY(len >= r.size())) {
        s->append((char *)r.data(), r.size());
        len -= r.size();
        ++it;
      } else {
        s->append((char *)r.data(), len);
        if (_consume) {
          it->skip_bytes(len);
        }
        len = 0;
        break;
      }
    }

    if (_consume && consume_begin != it) {
      _buffer->_fragments.erase_and_dispose(
          consume_begin, it, fragment_deleter);
    }

    _buffer->_size -= (size - len);
    return size - len;
  }

  usize buffer::reader::skip_any_of(u8 *pattern, usize len) {
    usize ret = 0;
    auto it   = _buffer->_fragments.begin();
    auto end  = _buffer->_fragments.end();

    while (end != it) {
      for (auto ch : it->data_range()) {
        if (::memchr(pattern, ch, len)) {
          goto done;
        }
        ++ret;
      }
    }
  done:
    if (_consume && ret) {
      _buffer->skip_bytes(ret);
    }
    return ret;
  }

  opt< usize > buffer::reader::find_byte(u8 target, usize offset) const {
    if (offset >= _buffer->size()) {
      return none;
    }

    auto it  = _buffer->_fragments.begin();
    auto end = _buffer->_fragments.end();

    for (; it != end && offset >= it->size(); ++it) {
      // find the first buffer past offset
      offset -= it->size();
    }

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
