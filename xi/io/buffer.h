#pragma once

#include "xi/io/fragment.h"

namespace xi {
namespace io {

  class buffer_allocator;

  class buffer final : public virtual ownership::unique {
    using list_t =
        intrusive::list< fragment,
                         intrusive::link_mode< intrusive::normal_link >,
                         intrusive::constant_time_size< true > >;
    list_t _fragments;
    usize _size = 0;

  public:
    struct iovec_adapter;
    class reader;

    buffer(own< fragment >);
    buffer(list_t &&, usize);
    ~buffer();
    XI_CLASS_DEFAULTS(buffer, move, ctor);

    void push_front(unique_ptr<buffer> &&, bool pack = false);
    void push_back(unique_ptr<buffer> &&, bool pack = false);

    bool empty() const;
    usize length() const;
    usize headroom() const;
    usize tailroom() const;
    usize size() const;

    unique_ptr<buffer> split(usize = -1);
    usize coalesce(mut< buffer_allocator > alloc,
                   usize offset = 0,
                   usize length = -1);
    byte_range range(mut< buffer_allocator > alloc,
                     usize offset = 0,
                     usize length = -1);

    void skip_bytes(usize, bool free = true);
    void ignore_bytes_at_end(usize, bool free = true);

    usize read(byte_range, usize offset = 0);
    usize write(const byte_range);
    unique_ptr<buffer> clone();
  };

  inline usize buffer::length() const {
    return _fragments.size();
  }
  inline usize buffer::size() const {
    return _size;
  }
}
}
