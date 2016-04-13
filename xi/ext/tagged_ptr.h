#pragma once

#include <cstdint>

#include "xi/ext/class_semantics.h"
#include "xi/ext/types.h"

namespace xi {
inline namespace ext {
  namespace detail {
    template < unsigned >
    struct bit_counter;

    template <>
    struct bit_counter< 1 > {
      enum { count = 1, mask = ~1 };
    };

    template <>
    struct bit_counter< 4 > {
      enum { count = 2, mask = ~3 };
    };

    template <>
    struct bit_counter< 8 > {
      enum { count = 3, mask = ~7 };
    };
  }

  template < class T, class E, usize align = alignof(void*) >
  class tagged_ptr {
    using bit_counter = detail::bit_counter< align >;
    template < E tag >
    constexpr static void verify_bit() noexcept {
      static_assert((tag < 1 << bit_counter::count),
                    "Tag value can not fit on current architecture.");
    }
    T* _pointer = nullptr;

  public:
    XI_CLASS_DEFAULTS(tagged_ptr, copy);

    explicit tagged_ptr(T* p = nullptr) : _pointer(p) {
    }

    explicit tagged_ptr(tagged_ptr&& other) : _pointer(other._pointer) {
      other._pointer = nullptr;
    }

    tagged_ptr& operator=(tagged_ptr&& other) {
      this->~tagged_ptr();
      new (this) tagged_ptr(move(other));
      return *this;
    }

    void reset(T* p) {
      _pointer = p;
    }

    template < E tag >
    void set_tag() {
      verify_bit< tag >();
      _pointer = reinterpret_cast< T* >(uintptr_t(_pointer) | tag);
    }

    template < E tag >
    void reset_tag() {
      verify_bit< tag >();
      _pointer = reinterpret_cast< T* >(uintptr_t(_pointer) & ~tag);
    }

    template < E tag >
    E get_tag() const {
      verify_bit< tag >();
      return static_cast< E >(uintptr_t(_pointer) & tag);
    }

    T* get() {
      return reinterpret_cast< T* >(uintptr_t(_pointer) & bit_counter::mask);
    }
    T const* get() const {
      return const_cast< T const* >(const_cast< tagged_ptr* >(this)->get());
    }

    T* operator->() {
      return get();
    }
    T const* operator->() const {
      return get();
    }
    explicit operator bool() const {
      return bool(get());
    }
  };
}
}
