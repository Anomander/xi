#pragma once

#include "xi/ext/common.h"
#include "xi/ext/exception.h"
#include "xi/ext/preprocessor.h"

namespace xi {
inline namespace ext {

  template < class action, bool (*condition)() >
  struct scope_action {
  private:
    action _action;

  public:
    scope_action(action &&a) : _action(move(a)) {
    }
    ~scope_action() {
      if (condition()) {
        _action();
      }
    }
  };

  template < bool (*condition)() >
  struct scope_action_builder {
  public:
    template < class A >
    auto operator<=(A &&a) {
      return scope_action< A, condition >(forward< A >(a));
    }
  };

  template < bool (*function)() >
  bool not__() {
    return !function();
  }
  inline bool always_true() {
    return true;
  }

  using success_Action_builder =
      scope_action_builder< &not__< &std::uncaught_exception > >;
  using failure_Action_builder =
      scope_action_builder< &std::uncaught_exception >;
  using fail_Action_builder  = failure_Action_builder;
  using error_Action_builder = failure_Action_builder;
  using exit_Action_builder  = scope_action_builder< &always_true >;
}
}

#define XI_SCOPE(when)                                                         \
  auto BOOST_PP_CAT($$__scope_##when##_var__$$, __COUNTER__) =                 \
      xi::BOOST_PP_CAT(when, _Action_builder)() <= [&]
