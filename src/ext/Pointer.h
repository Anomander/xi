#pragma once

#include "ext/Common.h"

#include <memory>

namespace xi {
inline namespace ext {

  using ::std::shared_ptr;
  using ::std::unique_ptr;
  using ::std::make_shared;
  using ::std::make_unique;
  using ::std::enable_shared_from_this;

  using ::std::static_pointer_cast;
  using ::std::dynamic_pointer_cast;

  template < class T, class U >
  auto dynamic_pointer_cast(unique_ptr< U >&& ptr) noexcept {
    auto cast = dynamic_cast< T* >(ptr.get());
    if (cast) {
      ptr.release();
    }
    return unique_ptr< T >(cast);
  }

  template < class T >
  auto makeSharedCopy(T&& t) {
    return make_shared< typename ::std::decay< T >::type >(forward< T >(t));
  }

  template < class T >
  auto makeUniqueCopy(T t) {
    return make_unique< typename ::std::decay< T >::type >(move(t));
  }

} // inline namespace ext
} // namespace xi
