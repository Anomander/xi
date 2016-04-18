#pragma once

#include "xi/ext/common.h"

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <memory>

namespace xi {
inline namespace ext {

  using ::std::shared_ptr;
  using ::std::weak_ptr;
  using ::std::unique_ptr;
  using ::std::make_shared;
  using ::std::make_unique;
  using ::std::enable_shared_from_this;

  using ::boost::intrusive_ptr;
  template < class T >
  using rc_counter =
      ::boost::intrusive_ref_counter< T, ::boost::thread_unsafe_counter >;
  template < class T >
  using arc_counter =
      ::boost::intrusive_ref_counter< T, ::boost::thread_safe_counter >;

  using ::std::static_pointer_cast;
  using ::std::dynamic_pointer_cast;

  template < class T, class U >
  auto dynamic_pointer_cast(unique_ptr< U > &&ptr) noexcept {
    auto cast = dynamic_cast< T * >(ptr.get());
    if (cast) {
      ptr.release();
    }
    return unique_ptr< T >(cast);
  }

  template < class T >
  auto make_shared_copy(T &&t) {
    return make_shared< typename ::std::decay< T >::type >(forward< T >(t));
  }

  template < class T >
  auto make_unique_copy(unique_ptr< T > &&t) {
    return t;
  }

  template < class T >
  auto make_unique_copy(T t) {
    return make_unique< typename ::std::decay< T >::type >(move(t));
  }

} // inline namespace ext
} // namespace xi
