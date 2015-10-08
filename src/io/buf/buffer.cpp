#include "xi/io/buf/buffer.h"
#include "xi/io/buf/view.h"

namespace xi {
namespace io {

  auto buffer::make_view(size_t offset, size_t length) const -> view {
    return view { share(_arena), _offset + offset, length };
  }
}
}
