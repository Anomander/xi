#pragma once

#include "xi/ext/test.h"
#include "xi/ext/type_traits.h"

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

    using false_type = ::boost::mpl::false_;
    using true_type = ::boost::mpl::true_;

    template < bool B > using bool_type = ::boost::mpl::bool_< B >;

    template < size_t A, size_t B >
    using ulong_greater =
        typename boost::mpl::greater< boost::mpl::integral_c< size_t, A >,
                                      boost::mpl::integral_c< size_t, B > >;

    enum class enabled {};

    template < class condition, class T = enabled >
    using enable_if = typename ::std::enable_if< condition::value, T >::type;

    template < class condition, class T = enabled >
    using disable_if = typename ::std::enable_if< !condition::value, T >::type;

    template < template < class > class, class... >
    struct multi_inherit_template;

    template < template < class > class base, class event, class... events >
    struct multi_inherit_template< base, event, events... >
        : public base< event >,
          public multi_inherit_template< base, events... > {
      virtual ~multi_inherit_template() noexcept = default;
    };

    // A simple class to describe a non-value
    struct null {};

    template < template < class > class base >
    struct multi_inherit_template< base > {};

    template < template < class... > class Template, class vector >
    struct variadic_template_from_vector {
      template < class V, size_t N, class... args > struct impl {
        using type = typename impl< typename pop_back< V >::type, N - 1,
                                    typename back< V >::type, args... >::type;
      };

      using type = typename impl< vector, size< vector >::value >::type;
    };

    template < template < class... > class Template, class vector >
    template < class V, class... args >
    struct variadic_template_from_vector< Template, vector >::impl< V, 0,
                                                                    args... > {
      using type = Template< args... >;
    };

    namespace _static_test {
      template < class... > class foo;
      STATIC_ASSERT_TEST(is_same<
          typename variadic_template_from_vector< foo, vector< int > >::type,
          foo< int > >);
      STATIC_ASSERT_TEST(is_same< typename variadic_template_from_vector<
                                      foo, vector< int, double > >::type,
                                  foo< int, double > >);
    }
  } // namespace meta
} // inline namespace ext
} // namespace xi
