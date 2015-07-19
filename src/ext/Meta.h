#pragma once

#include <boost/mpl/or.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/contains.hpp>

namespace xi {
inline namespace ext {
namespace meta {

using ::boost::mpl::or_;
using ::boost::mpl::vector;
using ::boost::mpl::contains;

using FalseType = ::boost::mpl::false_;
using TrueType = ::boost::mpl::true_;

enum class enabled {};

template < class Condition, class T = enabled >
using enable_if = typename ::std::enable_if< Condition::value, T >::type;

template < class Condition, class T = enabled >
using disable_if = typename ::std::enable_if< !Condition::value, T >::type;

template < template < class > class, class... > struct MultiInheritTemplate;

template < template < class > class Base, class Event, class... Events >
struct MultiInheritTemplate< Base, Event, Events... > : public Base< Event >,
                                                        public MultiInheritTemplate< Base, Events... > {
  virtual ~MultiInheritTemplate() noexcept = default;
};

template < template < class > class Base > struct MultiInheritTemplate< Base > {};

} // namespace meta
} // inline namespace ext
} // namespace xi
