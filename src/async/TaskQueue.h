#pragma once

#include "ext/Configure.h"
#include "ext/OverloadRank.h"
#include "util/PolymorphicSpScRingBuffer.h"

namespace xi {
namespace async {
  struct Task {
    virtual ~Task() = default;
    virtual void run() = 0;
  };

  class TaskQueue {
    PolymorphicSpScRingBuffer< Task > _ringBuffer;

    template < class Delegate >
    struct DelegateTask : public Task {
      Delegate _delegate;

      DelegateTask(Delegate const& d) : _delegate(d) {}
      DelegateTask(Delegate&& d) : _delegate(forward< Delegate >(d)) {}

      void run() override { _delegate(); }
    };

  public:
    TaskQueue(size_t sizeInBytes) : _ringBuffer(sizeInBytes) {}

    template < class Work >
    void submit(Work&& work) {
      _submit(forward< Work >(work), is_base_of< Task, Work >{});
    }

    void processTasks() {
      while (Task* task = _ringBuffer.next()) {
        task->run();
        _ringBuffer.pop();
      }
    }

  private:
    template < class Work >
    void _submit(Work&& work, meta::FalseType) {
      _ringBuffer.push(makeDelegateTask(forward< Work >(work)));
    }
    template < class Work >
    void _submit(Work&& work, meta::TrueType) {
      _ringBuffer.push(forward< Work >(work));
    }

    template < class Work >
    auto makeDelegateTask(Work&& work) {
      return DelegateTask< decay_t< Work > >(forward< Work >(work));
    }
  };
}
}
