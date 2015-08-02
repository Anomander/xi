#pragma once

#include "ext/Common.h"
#include "ext/Exception.h"

#include <boost/preprocessor.hpp>

namespace xi {
inline namespace ext {

  template < class Action, bool (*condition)() >
  struct ScopeAction {
  private:
    Action _action;

  public:
    ScopeAction(Action &&action) : _action(move(action)) {
    }
    ~ScopeAction() {
      if (condition())
        _action();
    }
  };

  template < bool (*condition)() >
  struct ScopeActionBuilder {
  public:
    template < class A >
    auto operator<=(A &&a) {
      return ScopeAction< A, condition >(forward< A >(a));
    }
  };

  template < bool (*function)() >
  bool not__() {
    return !function();
  }
  inline bool AlwaysTrue() {
    return true;
  }

  using success_ActionBuilder = ScopeActionBuilder< &not__< &std::uncaught_exception > >;
  using failure_ActionBuilder = ScopeActionBuilder< &std::uncaught_exception >;
  using fail_ActionBuilder = failure_ActionBuilder;
  using error_ActionBuilder = failure_ActionBuilder;
  using exit_ActionBuilder = ScopeActionBuilder< &AlwaysTrue >;
}
}

#define XI_SCOPE(when)                                                                                                 \
  auto BOOST_PP_CAT($$__scope_##when##_var__$$, __COUNTER__) = xi::BOOST_PP_CAT(when, _ActionBuilder)() <= [&]
