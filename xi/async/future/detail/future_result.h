#pragma once

#include "xi/async/future/forward_definitions.h"

namespace xi {
namespace async {

  template < class T > struct unwrapped_result_t { using type = T; };

  template < class T > struct unwrapped_result_t< future< T > > {
    using type = typename unwrapped_result_t< T >::type;
  };

  template <> struct unwrapped_result_t< void > { using type = meta::null; };

  template < class F, class... A > struct future_result_t {
    using type = typename unwrapped_result_t< result_of_t< F(A &&...) > >::type;
  };

  template < class F > struct future_result_t< F, void > {
    using type = typename unwrapped_result_t< result_of_t< F() > >::type;
  };

  template < class F > struct future_result_t< F, meta::null > {
    using type = typename unwrapped_result_t< result_of_t< F() > >::type;
  };

  template < class F, class... A >
  using future_result = typename future_result_t< F, A... >::type;

  STATIC_ASSERT_TEST(
      is_same< future_result< void (*)(void), void >, meta::null >);
  STATIC_ASSERT_TEST(is_same< future_result< void (*)(void) >, meta::null >);
  STATIC_ASSERT_TEST(
      is_same< future_result< void (*)(int), int >, meta::null >);
  STATIC_ASSERT_TEST(is_same<
      future_result< void (*)(int, double), int, double >, meta::null >);
  STATIC_ASSERT_TEST(
      is_same< future_result< meta::null (*)(void), void >, meta::null >);
  STATIC_ASSERT_TEST(
      is_same< future_result< meta::null (*)(void), void >, meta::null >);
  STATIC_ASSERT_TEST(is_same< future_result< int (*)(void), void >, int >);
  STATIC_ASSERT_TEST(is_same< future_result< int (*)(int), int >, int >);
}
}
