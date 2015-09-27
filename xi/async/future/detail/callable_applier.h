#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace async {

  template < class T > struct callable_applier {
  public:
    template < class func > static decltype(auto) apply(func &&f, T &&v) {
      return _apply(forward< func >(f), forward< T >(v),
                    is_same< void, result_of_t< func(T && ) > >{});
    }

  private:
    template < class func >
    static decltype(auto) _apply(func &&f, T &&v, meta::false_type) {
      return f(forward< T >(v));
    }
    template < class func >
    static meta::null _apply(func &&f, T &&v, meta::true_type) {
      f(forward< T >(v));
      return {};
    }
  };

  template <> struct callable_applier< meta::null > {
  public:
    template < class func >
    static decltype(auto) apply(func &&f, meta::null &&v) {
      return _apply(forward< func >(f),
                    is_same< void, result_of_t< func() > >{});
    }

  private:
    template < class func >
    static decltype(auto) _apply(func &&f, meta::false_type) {
      return f();
    }
    template < class func >
    static meta::null _apply(func &&f, meta::true_type) {
      f();
      return {};
    }
  };
}
}
