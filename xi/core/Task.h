#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace core {
  struct Task {
    virtual ~Task() = default;
    virtual void run() = 0;
  };

  template < class Func >
  auto makeTask(Func&& f) {
    struct DelegateTask : public Task {
      DelegateTask(Func&& delegate) : _delegate(forward< Func >(delegate)) {}
      void run() override { _delegate(); }

    private:
      decay_t< Func > _delegate;
    };

    return DelegateTask{forward< Func >(f)};
  }
}
}