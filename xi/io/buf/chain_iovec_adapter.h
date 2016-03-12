// #pragma once

// #include "xi/io/buf/chain.h"
// #include <sys/uio.h>

// namespace xi {
// namespace io {

//   class buffer::chain::iovec_adapter {
//   public:
//     static size_t fill_readable_iovec(ref< chain > c, iovec* iov, unsigned max,
//                                       unsigned start = 0) {
//       auto buf = c.head();
//       if (!is_valid(buf) || max == start) { return 0; }
//       unsigned i = start;
//       do {
//         if (!buf->is_empty()) {
//           tie(iov[i].iov_base, iov[i].iov_len) = buf->readable_head();
//           ++i;
//         }
//         buf = buf->_next;
//       } while (i < max && buf != c.head());
//       return i;
//     }
//     static size_t fill_writable_iovec(mut< chain > c, iovec* iov, unsigned max,
//                                       unsigned start = 0) {
//       auto buf = c->tail();
//       if (!is_valid(buf) || max == start) { return 0; }
//       unsigned i = start;
//       do {
//         tie(iov[i].iov_base, iov[i].iov_len) = buf->writable_tail();
//         ++i;
//         buf = buf->_next;
//       } while (i < max && buf != c->head());
//       return i;
//     }
//   };
// }
// }
