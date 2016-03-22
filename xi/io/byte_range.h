#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {

  // FIXME: you can now get a const string and make a mutable
  //        range out of it
  class byte_range {
    mutable u8 *_data   = nullptr;
    mutable usize _size = 0u;

  public:
    XI_CLASS_DEFAULTS(byte_range, copy, move);

    byte_range const &operator=(const byte_range &other) const {
      _data = other._data;
      _size = other._size;
      return *this;
    }

    bool empty() const {
      return _size == 0;
    }

    u8 *data() {
      return _data;
    }
    u8 const *data() const {
      return _data;
    }
    usize size() const {
      return _size;
    }

    u8 *begin() {
      return data();
    }
    u8 *end() {
      return data() + size();
    }

    u8 const *begin() const {
      return data();
    }
    u8 const *end() const {
      return data() + size();
    }

    explicit byte_range(void *p, usize l) noexcept
        : _data(reinterpret_cast< u8 * >(p)), _size(l) {
    }

    explicit byte_range(string &arr, usize sz) noexcept
        : byte_range((u8 *)arr.data(), min(sz, arr.size())) {
    }

    explicit byte_range(string const &arr, usize sz) noexcept
        : byte_range((u8 *)arr.data(), min(sz, arr.size())) {
    }

    explicit byte_range(string_ref arr, usize sz = -1) noexcept
        : byte_range((u8 *)arr.data(), min(sz, arr.size())) {
    }

    template < usize N >
    explicit byte_range(const char (&arr)[N], usize sz = -1) noexcept
        : byte_range((u8 *)arr, min(sz, N - 1)) {
    }

    template < class T >
    explicit byte_range(vector< T > &arr, usize sz = -1) noexcept
        : byte_range(reinterpret_cast< u8 * >(arr.data()),
                     min(sz, arr.size()) * sizeof(T)) {
    }

    template < class T >
    explicit byte_range(vector< T > const &arr, usize sz = -1) noexcept
        : byte_range(reinterpret_cast< u8 * >(arr.data()),
                     min(sz, arr.size()) * sizeof(T)) {
    }

    template < class T, usize S >
    explicit byte_range(array< T, S > &arr, usize sz = -1) noexcept
        : byte_range(reinterpret_cast< u8 * >(arr.data()),
                     min(sz, arr.size()) * sizeof(T)) {
    }

    bool operator==(byte_range const &other) const {
      return other._data == _data && other._size == _size;
    }

    static byte_range null() {
      return byte_range{nullptr, 0};
    }

    byte_range subrange(usize offset, usize length = -1) {
      if (offset >= _size) {
        return null();
      }
      if (XI_LIKELY(-1ull == length || length + offset > _size)) {
        length = _size - offset;
      }
      assert(offset + length <= _size);
      assert(make_signed_t< usize >(offset) >= 0);
      assert(make_signed_t< usize >(length) > 0);
      assert(make_signed_t< usize >(offset + length) > 0);
      return byte_range{_data + offset, length};
    }

    const byte_range subrange(usize offset, usize length = -1) const {
      return const_cast< byte_range * >(this)->subrange(offset, length);
    }

    string_ref to_string_ref() {
      return {(char *)_data, _size};
    }

    const string_ref to_string_ref() const {
      return {(char *)_data, _size};
    }
  };

  template < class T >
  const byte_range byte_range_for_object(T const &t) noexcept {
    return byte_range((u8 *)(&t), sizeof(T));
  }

  template < class T >
  byte_range byte_range_for_object(T &t) noexcept {
    return byte_range((u8 *)(&t), sizeof(T));
  }
}
}
