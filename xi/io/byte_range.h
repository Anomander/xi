#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {

  struct byte_range {
    uint8_t *data;
    size_t size;
    void consume(size_t sz) {
      auto actual = min(size, sz);
      data += actual;
      size -= actual;
    }
    bool empty() const { return size == 0; }
  };

  template < class T > auto byte_range_for_object(T &t) noexcept {
    return byte_range{reinterpret_cast< uint8_t * >(&t), sizeof(t)};
  }

  template < class T, size_t S >
  auto byte_range_for_object(array< T, S > &arr) noexcept {
    return byte_range{reinterpret_cast< uint8_t * >(arr.data()),
                      sizeof(T) * arr.size()};
  }
}
}
