#pragma once

#include <tuple>

#include "xi/ext/common.h"
#include "xi/ext/meta.h"

namespace xi {
inline namespace ext {

  using ::std::tuple;
  using ::std::tuple_size;
  using ::std::make_tuple;

  using ::std::piecewise_construct;
  using ::std::piecewise_construct_t;

  namespace detail {
    template < typename F, typename Tuple, size_t... I >
    decltype(auto) apply(F&& f, Tuple&& t, ::std::index_sequence< I... >) {
      return forward< F >(f)(::std::get< I >(forward< Tuple >(t))...);
    }
  }

  template < typename F, typename Tuple >
  decltype(auto) apply(F&& f, Tuple&& t) {
    using indices_t =
        ::std::make_index_sequence< tuple_size< decay_t< Tuple > >::value >;
    return detail::apply(forward< F >(f), forward< Tuple >(t), indices_t{});
  }
}
}
