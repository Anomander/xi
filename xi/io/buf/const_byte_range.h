#pragma once
#include "xi/io/buf/byte_range_base.h"

namespace xi {
namespace io {

  class const_byte_range : protected byte_range_base {
  public:
    const_byte_range() = default;
    const_byte_range(own< buf_base > buf, size_t offset, size_t length)
        : byte_range_base(move(buf), offset, length) {}
    const_byte_range(const_byte_range const &) = default;
    const_byte_range &operator=(const_byte_range const &) = default;
    const_byte_range(const_byte_range &&) = default;
    const_byte_range &operator=(const_byte_range &&) = default;

    const uint8_t *readable_bytes() const {
      return _buf->data() + _start_offset;
    }
    const uint8_t *cbegin() const { return readable_bytes(); }
    const uint8_t *cend() const { return cbegin() + _length; }
    uint32_t length() const { return _length; }
    bool is_empty() const { return 0 == _length; }
  };

}
}
