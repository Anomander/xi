// #pragma once
// #include "xi/io/buf/arena.h"

// namespace xi {
// namespace io {

//   class heap_buf : public arena {
//   public:
//     heap_buf(heap_buf &&) = default;
//     heap_buf &operator=(heap_buf &&) = default;

//     heap_buf(heap_buf const &) = delete;
//     heap_buf &operator=(heap_buf const &) = delete;

//     heap_buf(size_t sz) : arena(new uint8_t[sz], sz) {
//       std::cout << "Allocated " << sz << " bytes in [" << (void *)_data <<
//       "-"
//                 << (void *)(_data + _size) << "]" << std::endl;
//     }

//     ~heap_buf() {
//       delete[] data();
//       std::cout << "Deleted " << size() << " bytes";
//     }
//   };
// }
// }
