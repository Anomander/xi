#pragma once

#include "xi/ext/configure.h"
#include "xi/io/byte_range.h"
#include "xi/io/detail/byte_buffer_arena.h"

namespace xi {
namespace io {

  struct byte_buffer : public virtual ownership::unique {
    enum internal_t { kInternal };

    detail::byte_buffer_arena* _buffer;
    u8* _data;
    usize _length;
    u8* _head;
    usize _size;
    byte_buffer* _next = this;
    byte_buffer* _prev = this;

  public:
    ~byte_buffer();

    struct iovec_adapter;
    class chain;

  private:
    template < class > friend class basic_byte_buffer_allocator;

    byte_buffer(detail::byte_buffer_arena*, usize, usize);
    byte_buffer(internal_t, detail::byte_buffer_arena*, u8*, usize, u8*,
                usize) noexcept;

    XI_NEITHER_MOVABLE_NOR_COPIABLE(byte_buffer);

    u8* buffer_end() const;

  public:
    u8* data() const;
    u8* tail() const;
    usize headroom() const;
    usize size() const;
    usize tailroom() const;
    bool empty() const;

    byte_range head_range();
    const byte_range data_range() const;
    byte_range tail_range();

    void advance(usize);
    void retreat(usize);

    usize write(const byte_range);
    usize read(byte_range);
    usize discard_read(usize len);
    usize discard_write(usize len);

    unique_ptr< byte_buffer > clone();
  };
  static_assert(sizeof(byte_buffer) == 64, "");

  inline u8* byte_buffer::buffer_end() const { return _data + _length; }
  inline u8* byte_buffer::data() const { return _head; }
  inline u8* byte_buffer::tail() const { return _head + _size; }
  inline usize byte_buffer::headroom() const { return _head - _data; }
  inline usize byte_buffer::size() const { return _size; }
  inline bool byte_buffer::empty() const { return 0 == size(); }
  inline usize byte_buffer::tailroom() const { return buffer_end() - tail(); }
  inline byte_range byte_buffer::head_range() {
    return byte_range{_head, headroom()};
  }
  inline const byte_range byte_buffer::data_range() const {
    return byte_range{_head, size()};
  }
  inline byte_range byte_buffer::tail_range() {
    return byte_range{tail(), tailroom()};
  }
}
}
