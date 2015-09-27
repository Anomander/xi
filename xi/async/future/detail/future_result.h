#pragma once

#include "xi/async/future/forward_definitions.h"

namespace xi {
namespace async {

  template < class T > struct unwrapped_result_t { using type = T; };

  template < class T > struct unwrapped_result_t< future< T > > {
    using type = typename unwrapped_result_t< T >::type;
  };

  template <> struct unwrapped_result_t< void > { using type = meta::null; };

  template < class func, class T > struct future_result_t {
    using type = typename unwrapped_result_t< result_of_t< func(T &&)> >::type;
  };

  template < class func > struct future_result_t< func, void > {
    using type = typename unwrapped_result_t< result_of_t< func() > >::type;
  };

  template < class func > struct future_result_t< func, meta::null > {
    using type = typename unwrapped_result_t< result_of_t< func() > >::type;
  };

  template < class func, class T >
  using future_result = typename future_result_t< func, T >::type;

  STATIC_ASSERT_TEST(
      is_same< future_result< void (*)(void), void >, meta::null >);
  STATIC_ASSERT_TEST(
      is_same< future_result< void (*)(int), int >, meta::null >);
  STATIC_ASSERT_TEST(
      is_same< future_result< meta::null (*)(void), void >, meta::null >);
  STATIC_ASSERT_TEST(
      is_same< future_result< meta::null (*)(void), void >, meta::null >);
  STATIC_ASSERT_TEST(is_same< future_result< int (*)(void), void >, int >);
  STATIC_ASSERT_TEST(is_same< future_result< int (*)(int), int >, int >);
}
}
