#include "xi/io/byte_buffer_iovec_adapter.h"

#include <netinet/in.h>

namespace xi {
namespace io {

  usize byte_buffer::iovec_adapter::report_written(mut< byte_buffer > b,
                                                   usize sz) {
    auto change = min(b->tailroom(), sz);
    b->_size += change;
    return change;
  }

  void byte_buffer::iovec_adapter::fill_writable_iovec(mut< byte_buffer > b,
                                                       mut< iovec > iov) {
    iov->iov_base = b->tail();
    iov->iov_len = b->tailroom();
  }

  void byte_buffer::iovec_adapter::fill_readable_iovec(mut< byte_buffer > b,
                                                       mut< iovec > iov) {
    iov->iov_base = b->_data;
    iov->iov_len = b->size();
  }
}
}
