#pragma once

#include "xi/ext/Common.h"
#include "xi/ext/FastCast.h"
#include "xi/ext/OverloadRank.h"
#include "xi/ext/TypeTraits.h"

namespace xi {
inline namespace ext {
  namespace detail {

    template < class I, class T >
    struct Caster;

    template < class I, class T >
    struct Caster< I*, T* > {
      using type = add_pointer_t< I >;

      static auto cast(I* value) { return _cast(value, SelectRank); }

    protected:
      static auto _cast(I* value, Rank< 1 >) { return static_cast< T* >(value); }
      static auto _cast(I* value, Rank< 2 >) { return fast_cast< T* >(value); }
      static auto _cast(I* value, Rank< 3 >) { return dynamic_cast< T* >(value); }
    };

    template < class I, class T >
    struct Caster< I&, T& > {
      using type = add_lvalue_reference_t< I >;

      static auto cast(I& value) { return Caster< I*, T* >::cast(addressOf(value)); }
    };
  }

  namespace detail {
    template < class T, class I >
    auto cast(I obj, Rank< 1 >) -> decltype(static_cast< T >(obj)) {
      return static_cast< T >(obj);
    }
    template < class T, class I >
    auto cast(I* obj, Rank< 2 >) -> decltype(fast_cast< T >(obj)) {
      return fast_cast< T >(obj);
    }
#ifdef XI_CAST_ENABLE_DYNAMIC_CAST_BY_DEFAULT
    template < class T, class I >
    auto cast(I* obj, Rank< 3 >) -> decltype(dynamic_cast< T* >(obj)) {
      return dynamic_cast< T* >(obj);
    }
#endif // XI_CAST_ENABLE_DYNAMIC_CAST_BY_DEFAULT
  }
  template < class T, class I >
  auto cast(I obj) {
    return detail::cast< T >(obj, SelectRank);
  }

} // inline namespace ext
} // namespace xi
