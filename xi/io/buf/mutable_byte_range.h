#pragma once
#include "xi/io/buf/const_byte_range.h"

namespace xi {
namespace io {

  class mutable_byte_range : public const_byte_range {
  public:
    mutable_byte_range() = default;
    mutable_byte_range(own< buf_base > buf, size_t offset, size_t length)
        : const_byte_range(move(buf), offset, length) {}

    mutable_byte_range(mutable_byte_range const &) = delete;
    mutable_byte_range &operator=(mutable_byte_range const &) = delete;
    mutable_byte_range(mutable_byte_range &&) = default;
    mutable_byte_range &operator=(mutable_byte_range &&) = default;

    uint8_t *writable_bytes() { return _buf->data() + _start_offset; }
    uint8_t *begin() { return writable_bytes(); }
    uint8_t *end() { return begin() + _length; }

    mutable_byte_range split(size_t split_point) {
      if (split_point >= _length)
        return {share(_buf), _start_offset + _length, 0};
      XI_SCOPE(exit) { _length = split_point; };
      return {share(_buf), _start_offset + split_point, _length - split_point};
    }
  };
}
}
