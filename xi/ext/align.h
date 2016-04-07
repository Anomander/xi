#pragma once

#include "xi/ext/require.h"
#include "xi/ext/type_traits.h"
#include "xi/ext/types.h"

namespace xi {
inline namespace ext {

  template < typename T, XI_REQUIRE_DECL(is_integral< T >) >
  inline constexpr T align_up(T v, T align) {
    return (v + align - 1) & ~(align - 1);
  }

  template < typename T, XI_REQUIRE_DECL(meta::is_equal< sizeof(T), 1 >) >
  inline constexpr T* align_up(T* v, usize align) {
    return reinterpret_cast< T* >(
        align_up(reinterpret_cast< uintptr_t >(v), align));
  }

  template < typename T >
  inline constexpr T align_down(T v, T align) {
    return v & ~(align - 1);
  }

  template < typename T, XI_REQUIRE_DECL(meta::is_equal< sizeof(T), 1 >) >
  inline constexpr T* align_down(T* v, usize align) {
    return reinterpret_cast< T* >(
        align_down(reinterpret_cast< uintptr_t >(v), align));
  }

} // inline namespace ext
} // namespace xi
