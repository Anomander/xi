#pragma once

#include <boost/concept/requires.hpp>
#include <boost/concept_check.hpp>

namespace xi {
inline namespace ext {

  template < class T, class U = T >
  struct LessThanComparable {
    BOOST_CONCEPT_USAGE(LessThanComparable) {
      require_boolean_expr(a < b);
    }

  private:
    T a;
    U b;
  };
  template < class T, class U = T >
  struct EqualityComparable {
    BOOST_CONCEPT_USAGE(EqualityComparable) {
      require_boolean_expr(a == b);
    }

  private:
    T a;
    U b;
  };

#define XI_REQUIRE_CONCEPT(...)                                                \
  BOOST_CONCEPT_REQUIRES(((__VA_ARGS__)), (::xi::meta::enabled)) =             \
      ::xi::meta::enabled::value

#define XI_CONCEPT_ASSERT(...) BOOST_CONCEPT_ASSERT((__VA_ARGS__))
}
}
