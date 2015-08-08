#pragma once

#include "ext/Configure.h"
#include "async/Task.h"
#include "async/Engine.h"

namespace xi {
namespace async {
  struct FreeStoreTask : public Task, public virtual ownership::Unique {};

  template < class Func >
  auto makeFreeStoreTask(Func&& f) {
    struct DelegateTask : public FreeStoreTask {
      DelegateTask(Func&& delegate) : _delegate(forward< Func >(delegate)) {}
      void run() override { _delegate(); }

    private:
      Func _delegate;
    };

    return make< DelegateTask >(forward< Func >(f));
  }

  void schedule(own< FreeStoreTask > task) {
std::cout << "FreeStoreTask schedule" << std::endl;
    schedule([task = move(task)] { task->run(); });
  }
}
}
