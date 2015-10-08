#pragma once
#include "xi/ext/configure.h"

namespace xi {
namespace io {

  struct arena_base : public ownership::rc_shared {
    uint8_t *_data = nullptr;
    size_t _size = 0;
    size_t _consumed = 0;

  public:
    arena_base(uint8_t *data, size_t sz) : _data(data), _size(sz) {}
    uint8_t *data() { return _data; }
    size_t consumed() const { return _consumed; }
    size_t consume(size_t length) { return _consumed += length; }
    size_t size() const { return _size; }
    size_t remaining() const { return _size - _consumed; }
  };
}
}
