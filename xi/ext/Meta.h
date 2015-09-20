#pragma once

#include "xi/ext/Test.h"
#include "xi/ext/TypeTraits.h"

#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/pop_back.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/greater.hpp>

namespace xi {
inline namespace ext {
  namespace meta {

    using ::boost::mpl::if_;
    using ::boost::mpl::or_;
    using ::boost::mpl::vector;
    using ::boost::mpl::contains;
    using ::boost::mpl::push_back;
    using ::boost::mpl::pop_back;
    using ::boost::mpl::back;
    using ::boost::mpl::next;
    using ::boost::mpl::size;

    using FalseType = ::boost::mpl::false_;
    using TrueType = ::boost::mpl::true_;

    template < bool B >
    using BoolType = ::boost::mpl::bool_< B >;

    template < size_t A, size_t B >
    using ulongGreater =
        typename boost::mpl::greater< boost::mpl::integral_c< size_t, A >, boost::mpl::integral_c< size_t, B > >;

    enum class enabled {};

    template < class Condition, class T = enabled >
    using enable_if = typename ::std::enable_if< Condition::value, T >::type;

    template < class Condition, class T = enabled >
    using disable_if = typename ::std::enable_if< !Condition::value, T >::type;

    template < template < class > class, class... >
    struct MultiInheritTemplate;

    template < template < class > class Base, class Event, class... Events >
    struct MultiInheritTemplate< Base, Event, Events... > : public Base< Event >,
                                                            public MultiInheritTemplate< Base, Events... > {
      virtual ~MultiInheritTemplate() noexcept = default;
    };

    // A simple class to describe a non-value
    struct Null {};

    template < template < class > class Base >
    struct MultiInheritTemplate< Base > {};

    template < template < class... > class Template, class Vector >
    struct VariadicTemplateFromVector {
      template < class V, size_t N, class... Args >
      struct Impl {
        using type = typename Impl< typename pop_back< V >::type, N - 1, typename back< V >::type, Args... >::type;
      };

      using type = typename Impl< Vector, size< Vector >::value >::type;
    };

    template < template < class... > class Template, class Vector >
    template < class V, class... Args >
    struct VariadicTemplateFromVector< Template, Vector >::Impl< V, 0, Args... > {
      using type = Template< Args... >;
    };

    namespace _static_test {
      template < class... >
      class Foo;
      STATIC_ASSERT_TEST(is_same< typename VariadicTemplateFromVector< Foo, vector< int > >::type, Foo< int > >);
      STATIC_ASSERT_TEST(
          is_same< typename VariadicTemplateFromVector< Foo, vector< int, double > >::type, Foo< int, double > >);
    }
  } // namespace meta
} // inline namespace ext
} // namespace xi
