#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {
  struct task {
    virtual ~task()    = default;
    virtual void run() = 0;
  };

  template < class func >
  auto make_task(func &&f) {
    struct delegate_task : public task {
      delegate_task(func &&delegate) : _delegate(forward< func >(delegate)) {
      }
      void run() override {
        _delegate();
      }

    private:
      decay_t< func > _delegate;
    };

    return delegate_task{forward< func >(f)};
  }
}
}
