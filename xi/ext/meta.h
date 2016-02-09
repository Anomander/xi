#pragma once

#include "xi/ext/test.h"
#include "xi/ext/type_traits.h"

#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
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

    using false_type = ::std::integral_constant<bool, false>;
    using true_type = ::std::integral_constant<bool, true>;

    template < bool B > using bool_type = ::std::integral_constant< bool, B >;

    template < size_t A, size_t B >
    using ulong_greater =
        typename boost::mpl::greater< boost::mpl::integral_c< size_t, A >,
                                      boost::mpl::integral_c< size_t, B > >;

    enum class enabled { value };

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

  } // namespace meta
} // inline namespace ext
} // namespace xi
