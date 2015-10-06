#pragma once
#include "xi/io/buf/buf_base.h"

namespace xi {
namespace io {

  struct byte_range_base {
    own< buf_base > _buf;
    uint32_t _start_offset = 0;
    uint32_t _length = 0;

    byte_range_base() = default;
    byte_range_base(own< buf_base > buf, size_t offset, size_t length)
        : _buf(move(buf)), _start_offset(offset), _length(length) {
      // std::cout << "Created range of " << _length << " bytes in ["
      //           << (void *)(_buf->data() + _start_offset) << "-"
      //           << (void *)(_buf->data() + _start_offset + _length) << "]"
      //           << std::endl;
    }
  };

}
}
