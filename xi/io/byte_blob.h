#pragma once

#include "xi/ext/configure.h"
#include "xi/io/detail/guard.h"

namespace xi {
namespace io {

  class byte_blob {
  public:
    XI_CLASS_DEFAULTS(byte_blob, move, ctor);
    byte_blob(u8*, usize, detail::guard);
    byte_blob(ref< byte_blob > other);
    byte_blob& operator=(ref< byte_blob > other);
    u8* data();
    u8 const* data() const;
    u8* begin();
    u8 const* begin() const;
    u8 const* cbegin() const;
    u8* rbegin();
    u8 const* rbegin() const;
    u8* end();
    u8 const* end() const;
    u8 const* cend() const;
    u8* rend();
    u8 const* rend() const;
    usize size() const;
    bool empty() const;
    byte_blob clone();
    byte_blob split(usize offset);
    byte_blob share(usize offset = 0, usize length = -1);

  private:
    u8* _bytes  = nullptr;
    usize _size = 0;
    detail::guard _guard;
  };

  inline byte_blob::byte_blob(u8* b, usize s, detail::guard g)
      : _bytes(b), _size(s), _guard(move(g)) {
  }

  inline byte_blob::byte_blob(ref< byte_blob > other)
      : _bytes(other._bytes), _size(other._size), _guard(other._guard.share()) {
  }

  inline byte_blob& byte_blob::operator=(ref< byte_blob > other) {
    if (&other == this) {
      return *this;
    }
    this->~byte_blob();
    new (this) byte_blob(other);
    return *this;
  }

  inline u8* byte_blob::data() {
    return _bytes;
  }

  inline u8 const* byte_blob::data() const {
    return _bytes;
  }

  inline u8* byte_blob::begin() {
    return data();
  }

  inline u8 const* byte_blob::begin() const {
    return data();
  }

  inline u8 const* byte_blob::cbegin() const {
    return data();
  }

  inline u8* byte_blob::rbegin() {
    return end() - 1;
  }

  inline u8 const* byte_blob::rbegin() const {
    return end() - 1;
  }

  inline u8* byte_blob::end() {
    return data() + _size;
  }

  inline u8 const* byte_blob::end() const {
    return data() + _size;
  }

  inline u8 const* byte_blob::cend() const {
    return data() + _size;
  }

  inline u8* byte_blob::rend() {
    return begin() - 1;
  }

  inline u8 const* byte_blob::rend() const {
    return begin() - 1;
  }

  inline usize byte_blob::size() const {
    return _size;
  }

  inline bool byte_blob::empty() const {
    return !_size;
  }

  inline byte_blob byte_blob::clone() {
    return byte_blob{_bytes, _size, _guard.share()};
  }

  inline byte_blob byte_blob::split(usize offset) {
    assert(make_signed_t< usize >(offset) >= 0);
    if (offset >= _size) {
      return {};
    }
    if (offset == 0) {
      // move the entire blob
      XI_SCOPE(exit) {
        _guard = {};
        _bytes = nullptr;
        _size  = 0;
      };
      return byte_blob{_bytes, _size, move(_guard)};
    }
    XI_SCOPE(exit) {
      _size = offset;
    };
    return byte_blob{_bytes + offset, _size - offset, _guard.share()};
  }

  inline byte_blob byte_blob::share(usize offset, usize length) {
    if (offset >= _size || length == 0) {
      return {};
    }
    return byte_blob{
        _bytes + offset, min(length, _size - offset), _guard.share()};
  }
}
}
