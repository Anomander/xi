// #include "xi/io/buf/cursor.h"

// namespace xi {
// namespace io {

//   void buffer::chain::cursor::read(void* data, size_t len) {
//     if (0 == len || is_at_end()) { return; }
//     auto dst = (uint8_t*)data;
//     while (len >= _it.left_in_curr()) {
//       auto l = _it.left_in_curr();
//       copy(_it.pointer(), _it.end_of_curr(), dst);
//       _it += l;
//       dst += l;
//       len -= l;
//       if (is_at_end()) { return; }
//     }
//     if (len > 0) {
//       copy(_it.pointer(), _it.pointer() + len, dst);
//       _it += len;
//     }
//   }
// }
// }
