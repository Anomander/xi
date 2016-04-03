#include "xi/io/basic_buffer_allocator.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/byte_range.h"
#include "xi/io/detail/heap_buffer_storage_allocator.h"
#include "src/test/base.h"

namespace xi {
namespace test {

  struct buffer_base : public base {
    own< io::buffer_allocator > alloc = make< io::basic_buffer_allocator<
        io::detail::heap_buffer_storage_allocator > >();

  public:
    auto make_buffer(ref< string > in, usize headroom = 0, usize tailroom = 0) {
      auto b = alloc->allocate(in.size(), headroom, tailroom);
      b->write(io::byte_range{in});
      return move(b);
    }
    auto make_buffer(usize headroom, usize data, usize tailroom) {
      vector< u8 > in(data);
      usize i = 0u;
      std::generate_n(begin(in), data, [&] { return ++i; });

      auto b = alloc->allocate(data, headroom, tailroom);
      b->write(io::byte_range{in});
      return move(b);
    }
  };
}
}
