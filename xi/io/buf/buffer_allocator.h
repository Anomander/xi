#include "xi/ext/configure.h"
#include "xi/io/buf/buffer.h"

namespace xi {
namespace io {

  struct buffer_allocator : public virtual ownership::std_shared {
    virtual ~buffer_allocator() = default;
    /// Allocate range which has at least `size` bytes.
    /// Throws std::bad_alloc on failure.
    virtual own< buffer > allocate(size_t size) = 0;
    /// Reallocate range such that its size becomes at least `size`. Copy any
    /// data
    /// that's already in the range.
    /// Throws std::bad_alloc on failure.
    virtual own< buffer > reallocate(own< buffer >, size_t size) = 0;
  };
}
}
