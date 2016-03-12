#pragma once

#include "xi/io/detail/byte_buffer_storage_allocator.h"

namespace xi {
namespace io {
  namespace detail {

    class heap_byte_buffer_storage_allocator : public byte_buffer_storage_allocator {
    public:
      void* allocate(usize sz) override {
        auto m = malloc(sz);
        // std::cout << "malloc(" << m << ")" << std::endl;
        return m;
      }
      void free(void* p) override {
        // std::cout << "free(" << p << ")" << std::endl;
        ::free(p);
      }
    };

  }
}
}
