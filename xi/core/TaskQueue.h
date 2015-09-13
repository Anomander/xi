#pragma once

#include "xi/ext/Configure.h"
#include "xi/ext/Lockfree.h"
#include "xi/util/PolymorphicSpScRingBuffer.h"
#include "xi/util/SpinLock.h"
#include "xi/async/Task.h"

namespace xi {
namespace core {
  using async::Task;
  using async::makeTask;

  class TaskQueue : public virtual ownership::Unique {
    enum { kStaticOverflowQueueCapacity = 256, kRingBufferReserveSize = 32 };
    using OverflowQueueType = lockfree::queue< Task* >;

    struct DrainOverflowQueueTask : public Task {
      DrainOverflowQueueTask(mut< OverflowQueueType > q, mut< atomic< bool > > flag) : _queue(q), _flag(flag) {}
      ~DrainOverflowQueueTask() { std::cout << "Removing drain queue task." << std::endl; }
      void run() override {
        std::cout << "Running drain queue task." << std::endl;
        Task* task = nullptr;
        XI_SCOPE(success) { _flag->store(true); };
        while (_queue->pop(task) && task) {
          XI_SCOPE(exit) { delete task; };
          task->run();
        }
      }

    private:
      mut< OverflowQueueType > _queue;
      mut< atomic< bool > > _flag;
    };
    static_assert(sizeof(DrainOverflowQueueTask) == 24, "Must be 24");

    PolymorphicSpScRingBuffer< Task > _ringBuffer;
    alignas(64) OverflowQueueType _overflowQueue{kStaticOverflowQueueCapacity};
    alignas(64) atomic< bool > _overflowQueueDrained{true};

  public:
    TaskQueue(size_t sizeInBytes) : _ringBuffer(sizeInBytes, kRingBufferReserveSize) {}

    template < class Work >
    void submit(Work&& work) {
      _submit(forward< Work >(work), is_base_of< Task, decay_t< Work > >{});
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
      _pushTask(makeTask(forward< Work >(work)));
    }
    template < class Work >
    void _submit(Work&& work, meta::TrueType) {
      _pushTask(forward< Work >(work));
    }
    template < class T >
    void _pushTask(T&& t) {
      if (XI_UNLIKELY(!_ringBuffer.push(forward< T >(t)))) {
        {
          auto task = new decay_t< T >(forward< T >(t));
          XI_SCOPE(failure) { delete task; };
          _overflowQueue.push(task);
        }
        if (_overflowQueueDrained.load()) {
          std::cout << "Added drain queue task." << std::endl;
          if (XI_UNLIKELY(
                  !_ringBuffer.pushForced(DrainOverflowQueueTask(edit(_overflowQueue), edit(_overflowQueueDrained))))) {
            throw std::runtime_error("Unable to push overflow task onto queue.");
          }
          _overflowQueueDrained.store(false);
        }
      }
    }
  };
}
}
