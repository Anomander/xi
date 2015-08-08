#pragma once

#include "ext/Configure.h"
#include "async/Executor.h"
#include "async/TaskQueue.h"

namespace xi {
namespace async {

  template < class E >
  class ExecutorGroup : public virtual ownership::StdShared {
  public:
    ExecutorGroup(size_t count, size_t perCoreQueueSize) {
      _queues = new TaskQueue *[count];
      for (size_t dst = 0; dst < count; ++dst) {
        _queues[dst] = (TaskQueue *)malloc(sizeof(TaskQueue) * count);
        for (size_t src = 0; src < count; ++src) {
          new (&_queues[dst][src]) TaskQueue{perCoreQueueSize};
          std::cout << "Created queue " << src << "->" << dst << " @ " << &_queues[dst][src] << std::endl;
        }
      }
      for (size_t i = 0; i < count; ++i) {
        _executors.emplace_back(E{i, _queues, count});
      }
    }

    virtual ~ExecutorGroup() {
      auto n = _executors.size();
      _executors.clear();
      for (size_t dst = 0; dst < n; ++dst) {
        _queues[dst]->~TaskQueue();
        free(_queues[dst]);
      }
      delete[] _queues;
    }

    template < class Func >
    void run(Func f) {
      for (auto &e : _executors) {
        e.run(f);
      }
      for (auto &e : _executors) {
        e.join();
      }
    }

    template < class Func >
    void executeOnAll(Func f) {
      for (auto &e : _executors) {
        e.submit(f);
      }
    }

    template < class Func >
    void executeOn(size_t core, Func &&f) {
      if (core > _executors.size()) {
        throw std::invalid_argument("Core not registered");
      }
      _executors[core].submit(forward< Func >(f));
    }

    template < class Func >
    void executeOnNext(Func &&f) {
      _executors[(_nextCore++) % _executors.size()].submit(forward< Func >(f));
    }

  private:
    vector< E > _executors;
    TaskQueue **_queues;
    size_t _nextCore = 0;
  };

}
}
