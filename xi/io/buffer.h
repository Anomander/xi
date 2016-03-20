#pragma once

#include "xi/io/fragment.h"

namespace xi {
namespace io {

  class buffer_allocator;

  class buffer final {
    using list_t =
        intrusive::list< fragment,
                         intrusive::link_mode< intrusive::normal_link >,
                         intrusive::constant_time_size< true > >;
    list_t _fragments;
    usize _size = 0;

  private:
    buffer(list_t &&, usize);

  public:
    struct iovec_adapter;

    buffer() = default;
    buffer(own< fragment >);
    ~buffer();
    XI_CLASS_DEFAULTS(buffer, move);

    void push_front(buffer &&, bool pack = false);
    void push_back(buffer &&, bool pack = false);

    bool empty() const;
    usize length() const;
    usize headroom() const;
    usize tailroom() const;
    usize size() const;

    buffer split(usize = -1);
    usize coalesce(mut< buffer_allocator > alloc, usize offset = 0,
                   usize length = -1);

    void skip_bytes(usize, bool free = true);
    void ignore_bytes_at_end(usize, bool free = true);

    usize read(byte_range);
    usize write(const byte_range);

    template < class T >
    opt< T > read() {
      if (size() >= sizeof(T)) {
        T value;
        read(byte_range_for_object(value));
        skip_bytes(sizeof(T));
        return some(value);
      }
      return none;
    }
  };

  inline usize buffer::length() const {
    return _fragments.size();
  }
  inline usize buffer::size() const {
    return _size;
  }
}
}
