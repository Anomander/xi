#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace io {

  struct ByteRange {
    uint8_t* data;
    size_t size;
    void consume(size_t sz) {
      auto actual = min(size, sz);
      data += actual;
      size -= actual;
    }
    bool empty() const {
      return size == 0;
    }
  };

  template<class T>
  auto byteRangeForObject(T & t) noexcept {
    return ByteRange { reinterpret_cast<uint8_t*>(& t), sizeof(t) };
  }

  template<class T, size_t S>
  auto byteRangeForObject(array<T,S> & arr) noexcept {
    return ByteRange { reinterpret_cast<uint8_t*>(arr.data()), sizeof(T) * arr.size() };
  }
}
}
