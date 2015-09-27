#pragma once

#include "xi/ext/type_traits.h"

#include <boost/optional.hpp>

namespace xi {
inline namespace ext {

  using ::boost::optional;
  using ::boost::none;

  template < class T > using opt = optional< T >;

  template < class T, class F >
  auto fmap(optional< T > val, F &&f) -> ::std::result_of_t< F(optional< T >)> {
    if (val) { return f(val); } else {
      return none;
    }
  }

} // inline namespace ext
} // namespace xi
