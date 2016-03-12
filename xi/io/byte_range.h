#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {

  class byte_range {
    u8 *_data = nullptr;
    usize _size = 0u;

  public:
    bool empty() const { return _size == 0; }

    u8 *data() { return _data; }
    u8 const *data() const { return _data; }
    usize size() const { return _size; }

    u8 *begin() { return data(); }
    u8 *end() { return data() + size(); }

    u8 const *begin() const { return data(); }
    u8 const *end() const { return data() + size(); }

    explicit byte_range(void *p, usize l) noexcept
        : _data(reinterpret_cast< u8 * >(p)),
          _size(l) {}

    template < class T >
    explicit byte_range(T &t) noexcept : _data(reinterpret_cast< u8 * >(&t)),
                                         _size(sizeof(T)) {}

    explicit byte_range(string &arr, usize sz = -1) noexcept
        : _data((u8 *)arr.data()),
          _size(min(sz, arr.size())) {}

    explicit byte_range(string_ref &arr, usize sz = -1) noexcept
        : _data((u8 *)arr.data()),
          _size(min(sz, arr.size())) {}

    template < class T >
    explicit byte_range(vector< T > &arr, usize sz = -1) noexcept
        : _data(reinterpret_cast< u8 * >(arr.data())),
          _size(min(sz, arr.size()) * sizeof(T)) {}

    template < class T, usize S >
    explicit byte_range(array< T, S > &arr, usize sz = -1) noexcept
        : _data(reinterpret_cast< u8 * >(arr.data())),
          _size(min(sz, arr.size()) * sizeof(T)) {}
  };
}
}
