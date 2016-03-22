#pragma once

#include <cstdint>

namespace xi {
inline namespace ext {
  namespace detail {
    template < unsigned >
    struct bit_counter;

    template <>
    struct bit_counter< 4 > {
      enum { count = 2, mask = ~3 };
    };

    template <>
    struct bit_counter< 8 > {
      enum { count = 3, mask = ~7 };
    };
  }

  template < class T, class E >
  class tagged_ptr {
    using bit_counter = detail::bit_counter< sizeof(void*) >;
    template < E tag >
    constexpr void verify_bit() noexcept {
      static_assert((tag < 1 << bit_counter::count),
                    "Tag value can not fit on current architecture.");
    }
    T* _pointer = nullptr;

  public:
    tagged_ptr(T* p = nullptr) : _pointer(p) {
    }
    tagged_ptr(tagged_ptr const&) = default;
    tagged_ptr(tagged_ptr&&)      = default;

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
    E get_tag() {
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
  };
}
}
