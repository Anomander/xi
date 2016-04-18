#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {

  template < class... A >
  struct callable_applier {
  public:
    template < class F >
    static decltype(auto) apply(F &&f, A &&... args) {
      return _apply(forward< F >(f),
                    forward< A >(args)...,
                    is_same< void, result_of_t< F(A && ...) > >{});
    }

  private:
    template < class F >
    static decltype(auto) _apply(F &&f, A &&... args, meta::false_type) {
      return f(forward< A >(args)...);
    }
    template < class F >
    static meta::null _apply(F &&f, A &&... args, meta::true_type) {
      f(forward< A >(args)...);
      return {};
    }
  };

  template <>
  struct callable_applier< meta::null > {
  public:
    template < class F >
    static decltype(auto) apply(F &&f, meta::null &&) {
      return _apply(forward< F >(f), is_same< void, result_of_t< F() > >{});
    }
    template < class F >
    static decltype(auto) apply(F &&f) {
      return _apply(forward< F >(f), is_same< void, result_of_t< F() > >{});
    }

  private:
    template < class F >
    static decltype(auto) _apply(F &&f, meta::false_type) {
      return f();
    }
    template < class F >
    static meta::null _apply(F &&f, meta::true_type) {
      f();
      return {};
    }
  };
}
}
