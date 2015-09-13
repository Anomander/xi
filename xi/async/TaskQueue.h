#pragma once

#include "xi/ext/Configure.h"
#include "xi/ext/OverloadRank.h"
#include "xi/util/PolymorphicSpScRingBuffer.h"
#include "xi/async/Task.h"

namespace xi {
namespace async {

  class TaskQueue : public virtual ownership::Unique {
    PolymorphicSpScRingBuffer< Task > _ringBuffer;

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
      _ringBuffer.push(makeTask(forward< Work >(work)));
    }
    template < class Work >
    void _submit(Work&& work, meta::TrueType) {
      _ringBuffer.push(forward< Work >(work));
    }
  };
}
}
