#pragma once

#include "ext/Configure.h"

namespace xi {
  namespace io {

    class Buffer
      : public ownership::Unique
    {
    public:

      Buffer (size_t size)
        : _size (size)
      {}

    private:
      size_t _size;
      uint8_t _data[0];
    };

    own<Buffer> create (size_t sz) {
      uint8_t * bytes = reinterpret_cast<uint8_t *> malloc (sizeof (Buffer) + sz);
      return new (bytes) Buffer(sz);
    }
  }
}
