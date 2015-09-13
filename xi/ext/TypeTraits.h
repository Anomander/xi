#pragma once

#include "xi/ext/Pointer.h"

#include <type_traits>
#include <boost/type_traits.hpp>

namespace xi {
inline namespace ext {

  using ::std::is_const;
  using ::std::add_lvalue_reference_t;
  using ::std::remove_reference_t;
  using ::std::add_pointer_t;
  using ::std::remove_pointer_t;
  using ::std::decay_t;
  using ::std::add_const_t;
  using ::std::remove_const_t;
  using ::std::remove_cv_t;
  using ::std::result_of_t;

  using ::boost::is_same;
  using ::boost::is_base_of;

  using ::std::aligned_storage_t;

  template < class Src, class Dst >
  struct copy_const {
    using type = remove_const_t< Dst >;
  };
  template < class Src, class Dst >
  struct copy_const< const Src, Dst > {
    using type = add_const_t< Dst >;
  };
  template < class Src, class Dst >
  using copy_const_t = typename copy_const< const Src, Dst >::type;

  namespace detail {
    template < class T >
    struct AddressOf {
      using type = decltype(::std::addressof(*((const T *)0)));
      static type value(T const &t) { return ::std::addressof(t); }
    };
    template < class T >
    struct AddressOf< T * > {
      using type = T *;
      static type value(T *const &t) { return t; }
    };
    template < class T >
    struct AddressOf< shared_ptr< T > > {
      using type = T *;
      static type value(shared_ptr< T > const &t) { return t.get(); }
    };
    template < class T >
    struct AddressOf< unique_ptr< T > > {
      using type = T *;
      static type value(unique_ptr< T > const &t) { return t.get(); }
    };
  }
  template < class T >
  using addressOf_t = typename detail::AddressOf< T >::type;
  template < class T >
  auto addressOf(T const &t) {
    return detail::AddressOf< T >::value(t);
  }

} // inline namespace ext
} // namespace xi
