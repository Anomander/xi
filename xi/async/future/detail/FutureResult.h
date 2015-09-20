#pragma once

#include "xi/async/future/ForwardDefinitions.h"

namespace xi {
namespace async {

  template < class T >
  struct UnwrappedResult_t {
    using type = T;
  };

  template < class T >
  struct UnwrappedResult_t< Future< T > > {
    using type = typename UnwrappedResult_t< T >::type;
  };

  template <>
  struct UnwrappedResult_t< void > {
    using type = meta::Null;
  };

  template < class Func, class T >
  struct FutureResult_t {
    using type = typename UnwrappedResult_t< result_of_t< Func(T&&)> >::type;
  };

  template < class Func >
  struct FutureResult_t< Func, void > {
    using type = typename UnwrappedResult_t< result_of_t< Func() > >::type;
  };

  template < class Func >
  struct FutureResult_t< Func, meta::Null > {
    using type = typename UnwrappedResult_t< result_of_t< Func() > >::type;
  };

  template < class Func, class T >
  using FutureResult = typename FutureResult_t< Func, T >::type;

  STATIC_ASSERT_TEST(is_same< FutureResult< void (*)(void), void >, meta::Null >);
  STATIC_ASSERT_TEST(is_same< FutureResult< void (*)(int), int >, meta::Null >);
  STATIC_ASSERT_TEST(is_same< FutureResult< meta::Null (*)(void), void >, meta::Null >);
  STATIC_ASSERT_TEST(is_same< FutureResult< meta::Null (*)(void), void >, meta::Null >);
  STATIC_ASSERT_TEST(is_same< FutureResult< int (*)(void), void >, int >);
  STATIC_ASSERT_TEST(is_same< FutureResult< int (*)(int), int >, int >);
}
}
