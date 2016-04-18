#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {
  struct task : public virtual ownership::unique {
    virtual ~task()    = default;
    virtual void run() = 0;
  };

  inline auto make_task(own<task> t) {
    return t;
  }

  template < class T, XI_REQUIRE_DECL(is_base_of<task, decay_t<T>>) >
  auto make_task(T &&t) {
    return t;
  }

  template < class F, XI_UNLESS_DECL(is_base_of<task, decay_t<F>>) >
  auto make_task(F &&f) {
    struct delegate_task : public task {
      delegate_task(F &&delegate) : _delegate(forward< F >(delegate)) {
      }

      void run() override {
        _delegate();
      }

    private:
      decay_t< F > _delegate;
    };

    return delegate_task{forward< F >(f)};
  }
}
}
