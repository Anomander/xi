#pragma once

#include "xi/ext/common.h"
#include "xi/ext/fast_cast.h"
#include "xi/ext/overload_rank.h"
#include "xi/ext/type_traits.h"

namespace xi {
inline namespace ext {
  namespace detail {

    template < class I, class T >
    struct caster;

    template < class I, class T >
    struct caster< I *, T * > {
      using type = add_pointer_t< I >;

      static auto cast(I *value) {
        return _cast(value, select_rank);
      }

    protected:
      static auto _cast(I *value, rank< 1 >) {
        return static_cast< T * >(value);
      }
      static auto _cast(I *value, rank< 2 >) {
        return fast_cast< T * >(value);
      }
      static auto _cast(I *value, rank< 3 >) {
        return dynamic_cast< T * >(value);
      }
    };

    template < class I, class T >
    struct caster< I &, T & > {
      using type = add_lvalue_reference_t< I >;

      static auto cast(I &value) {
        return caster< I *, T * >::cast(address_of(value));
      }
    };
  }

  namespace detail {
    template < class T, class I >
    auto cast(I obj, rank< 1 >) -> decltype(static_cast< T >(obj)) {
      return static_cast< T >(obj);
    }
    template < class T, class I >
    auto cast(I *obj, rank< 2 >) -> decltype(fast_cast< T >(obj)) {
      return fast_cast< T >(obj);
    }
#ifdef XI_CAST_ENABLE_DYNAMIC_CAST_BY_DEFAULT
    template < class T, class I >
    auto cast(I *obj, rank< 3 >) -> decltype(dynamic_cast< T * >(obj)) {
      return dynamic_cast< T * >(obj);
    }
#endif // XI_CAST_ENABLE_DYNAMIC_CAST_BY_DEFAULT
  }
  template < class T, class I >
  auto cast(I obj) {
    return detail::cast< T >(obj, select_rank);
  }

} // inline namespace ext
} // namespace xi
