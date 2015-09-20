#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace async {

  template < class T >
  struct CallableApplier {
  public:
    template < class Func >
    static decltype(auto) apply(Func&& f, T&& v) {
      return _apply(forward< Func >(f), forward< T >(v), is_same< void, result_of_t< Func(T && ) > >{});
    }

  private:
    template < class Func >
    static decltype(auto) _apply(Func&& f, T&& v, meta::FalseType) {
      return f(forward< T >(v));
    }
    template < class Func >
    static meta::Null _apply(Func&& f, T&& v, meta::TrueType) {
      f(forward< T >(v));
      return {};
    }
  };

  template <>
  struct CallableApplier< meta::Null > {
  public:
    template < class Func >
    static decltype(auto) apply(Func&& f, meta::Null&& v) {
      return _apply(forward< Func >(f), is_same< void, result_of_t< Func() > >{});
    }

  private:
    template < class Func >
    static decltype(auto) _apply(Func&& f, meta::FalseType) {
      return f();
    }
    template < class Func >
    static meta::Null _apply(Func&& f, meta::TrueType) {
      f();
      return {};
    }
  };
}
}
