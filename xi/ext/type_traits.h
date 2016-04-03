#pragma once

#include "xi/ext/pointer.h"

#include <type_traits>

namespace xi {
inline namespace ext {

  using ::std::is_const;
  using ::std::is_arithmetic;

  using ::std::add_lvalue_reference_t;
  using ::std::add_rvalue_reference_t;
  using ::std::remove_reference_t;
  using ::std::add_pointer_t;
  using ::std::remove_pointer_t;
  using ::std::decay_t;
  using ::std::add_const_t;
  using ::std::remove_const_t;
  using ::std::remove_cv_t;
  using ::std::result_of_t;

  using ::std::is_same;
  using ::std::is_base_of;
  using ::std::is_arithmetic;
  using ::std::is_integral;
  using ::std::is_unsigned;
  using ::std::is_signed;
  using ::std::has_virtual_destructor;
  using ::std::is_nothrow_destructible;

  using ::std::aligned_storage_t;

  using ::std::make_signed_t;
  using ::std::make_unsigned_t;

  template < class T, class Signature >
  struct is_callable;
  template < class T, class Ret, class... Args >
  struct is_callable< T, Ret(Args...) > {
    enum { value = is_same< Ret, result_of_t< T(Args...) > >::value };
  };

  template < class src, class dst >
  struct copy_const {
    using type = remove_const_t< dst >;
  };
  template < class src, class dst >
  struct copy_const< const src, dst > {
    using type = add_const_t< dst >;
  };
  template < class src, class dst >
  using copy_const_t = typename copy_const< const src, dst >::type;

  namespace detail {
    template < class T >
    struct address_of {
      using type = decltype(::std::addressof(*((const T *)0)));
      static type value(T const &t) {
        return ::std::addressof(t);
      }
    };
    template < class T >
    struct address_of< T * > {
      using type = T *;
      static type value(T *const &t) {
        return t;
      }
    };
    template < class T >
    struct address_of< shared_ptr< T > > {
      using type = T *;
      static type value(shared_ptr< T > const &t) {
        return t.get();
      }
    };
    template < class T >
    struct address_of< unique_ptr< T > > {
      using type = T *;
      static type value(unique_ptr< T > const &t) {
        return t.get();
      }
    };
  }
  template < class T >
  using address_of_t = typename detail::address_of< T >::type;
  template < class T >
  auto address_of(T const &t) {
    return detail::address_of< T >::value(t);
  }

} // inline namespace ext
} // namespace xi
