#include "xi/io/buf/buffer.h"
#include "xi/io/buf/view.h"

namespace xi {
namespace io {

  buffer::buffer()
      : _arena(), _offset(0), _capacity(0), _begin(nullptr), _end(_begin) {}

  buffer::buffer(own< arena_base > arena, size_t offset, size_t len)
      : _arena(move(arena))
      , _offset(offset)
      , _capacity(len)
      , _begin(_arena->data() + _offset)
      , _end(_begin) {
    if (offset + len > _arena->size() || offset > _arena->size() ||
        len > _arena->size()) {
      throw std::out_of_range("Not enough space.");
    }
    std::cout << "Allocated buffer of " << _capacity << " bytes in ["
              << (void*)_begin << "-" << (void*)(_begin + _capacity) << "]"
              << std::endl;
  }

  buffer::~buffer() {
    std::cout << "Destroyed buffer of " << _capacity << " bytes in ["
              << (void*)_begin << "-" << (void*)(_begin + _capacity) << "]"
              << std::endl;
  }

  auto buffer::make_view(size_t offset, size_t length) const -> view {
    return view{share(_arena), _offset + offset, length};
  }
}
}
