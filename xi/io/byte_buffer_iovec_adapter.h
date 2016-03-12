#pragma once

#include "xi/io/byte_buffer.h"

struct iovec;

namespace xi {
namespace io {

  struct byte_buffer::iovec_adapter {
    static usize report_written(mut< byte_buffer > b, usize sz);
    static void fill_writable_iovec(mut< byte_buffer > b, mut< iovec > iov);
    static void fill_readable_iovec(mut< byte_buffer > b, mut< iovec > iov);
  };
}
}
