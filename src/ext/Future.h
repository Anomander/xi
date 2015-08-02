#pragma once

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#include <boost/thread.hpp>

#include "ext/Configure.h"

namespace xi {
inline namespace ext {

#define METHOD_FORWARD(name, method)                                                                                   \
  template < class... Args >                                                                                           \
  auto name(Args&&... args) {                                                                                          \
    return method(forward< Args >(args)...);                                                                           \
  }

  template < class T >
  struct Future : public ::boost::future< T > {
    using Base = ::boost::future< T >;
    using Base::future;
    Future(Base&& f) : Base(::std::forward< Base >(f)) {
    }
  };

  template < class T >
  struct Promise : public ::boost::promise< T >, virtual public ownership::StdShared {
    using Base = ::boost::promise< T >;
    using Base::promise;

    METHOD_FORWARD(setValue, set_value);
    METHOD_FORWARD(getFuture, get_future);

  private: // renamed methods
    using Base::get_future;
    using Base::set_value;
  };
  using namespace ::boost::future_state;

} // inline namespace ext
} // namespace xi
