#pragma once

#include "xi/ext/require.h"
#include "xi/ext/type_traits.h"

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/irange.hpp>

namespace xi {
inline namespace ext {
  namespace adaptors {
    using ::boost::adaptors::reverse;
  } // namespace adaptors

  namespace range {
    namespace detail {
      template < class T, XI_REQUIRE_DECL(is_integral< T >) >
      struct step_value {
        T value;
      };
    }
    template < typename T, XI_REQUIRE_DECL(is_integral< T >) >
    auto step(T value) {
      return detail::step_value<T>{ value };
    }
    template < typename T, XI_REQUIRE_DECL(is_integral< T >) >
    auto to(T upper) {
      return ::boost::irange(static_cast< T >(0), upper);
    }
    template < typename T, XI_REQUIRE_DECL(is_integral< T >) >
    auto to(T upper, detail::step_value<T> s) {
      return ::boost::irange(static_cast< T >(0), upper, s.value);
    }
    template < typename T, XI_REQUIRE_DECL(is_integral< T >) >
    auto between(T lower, T upper) {
      return ::boost::irange(lower, upper);
    }
    template < typename T, XI_REQUIRE_DECL(is_integral< T >) >
    auto between(T lower, T upper, detail::step_value<T> s) {
      return ::boost::irange(lower, upper, s.value);
    }
  }
} // inline namespace ext
} // namespace xi
