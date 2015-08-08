#pragma once

#include "ext/Configure.h"
#include "async/Context.h"

namespace xi {
namespace async {
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
      Func _delegate;
    };

    return DelegateTask{forward< Func >(f)};
  }
}
}
