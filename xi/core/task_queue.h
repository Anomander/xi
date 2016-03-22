#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/lockfree.h"
#include "xi/core/task.h"
#include "xi/util/polymorphic_sp_sc_ring_buffer.h"
#include "xi/util/spin_lock.h"

namespace xi {
namespace core {

  class task_queue : public virtual ownership::unique {
    enum { kStaticOverflowQueueCapacity = 256, kRingBufferReserveSize = 32 };
    using overflow_queue_type = lockfree::queue< task * >;

    struct drain_overflow_queue_task : public task {
      drain_overflow_queue_task(mut< overflow_queue_type > q,
                                mut< atomic< bool > > flag)
          : _queue(q), _flag(flag) {
      }
      ~drain_overflow_queue_task() {
        std::cout << "Removing drain queue task." << std::endl;
      }
      void run() override {
        std::cout << "Running drain queue task." << std::endl;
        task *task = nullptr;
        XI_SCOPE(success) {
          _flag->store(true);
        };
        while (_queue->pop(task) && task) {
          XI_SCOPE(exit) {
            delete task;
          };
          task->run();
        }
      }

    private:
      mut< overflow_queue_type > _queue;
      mut< atomic< bool > > _flag;
    };
    static_assert(sizeof(drain_overflow_queue_task) == 24, "Must be 24");

    polymorphic_sp_sc_ring_buffer< task > _ring_buffer;
    alignas(64) overflow_queue_type _overflow_queue{
        kStaticOverflowQueueCapacity};
    alignas(64) atomic< bool > _overflow_queue_drained{true};

  public:
    task_queue(size_t size_in_bytes)
        : _ring_buffer(size_in_bytes, kRingBufferReserveSize) {
    }

    template < class W >
    void submit(W &&work) {
      _submit(forward< W >(work), is_base_of< task, decay_t< W > >{});
    }

    void process_tasks() {
      while (task *task = _ring_buffer.next()) {
        task->run();
        _ring_buffer.pop();
      }
    }

  private:
    template < class W >
    void _submit(W &&work, meta::false_type) {
      _push_task(make_task(forward< W >(work)));
    }
    template < class W >
    void _submit(W &&work, meta::true_type) {
      _push_task(forward< W >(work));
    }
    template < class T >
    void _push_task(T &&t) {
      if (XI_UNLIKELY(!_ring_buffer.push(forward< T >(t)))) {
        {
          auto task = new decay_t< T >(forward< T >(t));
          XI_SCOPE(failure) {
            delete task;
          };
          _overflow_queue.push(task);
        }
        if (_overflow_queue_drained.load()) {
          std::cout << "Added drain queue task." << std::endl;
          if (XI_UNLIKELY(!_ring_buffer.push_forced(drain_overflow_queue_task(
                  edit(_overflow_queue), edit(_overflow_queue_drained))))) {
            throw std::runtime_error(
                "Unable to push overflow task onto queue.");
          }
          _overflow_queue_drained.store(false);
        }
      }
    }
  };
}
}
