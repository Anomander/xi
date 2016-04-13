#include "xi/core/shard.h"
#include "xi/core/kernel.h"

namespace xi {
namespace core {
  class deferred_poller : public poller {
    deque< own< task > > _deferred_tasks;
    mut< task_queue > _task_queue;

  public:
    deferred_poller(mut< task_queue > tq) : _task_queue(tq) {
    }

    void push(own< task > t) {
      _deferred_tasks.emplace_back(move(t));
    }

    unsigned poll() noexcept override {
      for (auto &&t : _deferred_tasks) {
        _task_queue->submit(move(t));
      }
      _deferred_tasks.clear();
      return 0;
    }
  };

  class reactor_poller : public poller {
    own< core::reactor > _reactor;

  public:
    reactor_poller(own< core::reactor > reactor) : _reactor(reactor) {
    }
    unsigned poll() noexcept override {
      try {
        _reactor->poll();
      } catch (...) {
      }
      return 0;
    }
  };

  thread_local mut< shard > this_shard = nullptr;

  shard::shard(mut< kernel > k) : _core_id(0), _kernel(k) {
    std::cout << "Starting shard @" << _core_id << " in thread "
              << pthread_self() << std::endl;
  }

  void shard::start() {
    auto dp          = make< deferred_poller >(edit(_task_queue));
    _deferred_poller = edit(dp);
    register_poller(move(dp));
  }

  void shard::attach_reactor(own< core::reactor > r) {
    _reactor = edit(r);
    register_poller(make< reactor_poller >(move(r)));
  }

  usize shard::register_poller(own< poller > poller) {
    auto ret = reinterpret_cast< size_t >(address_of(poller));
    dispatch([ poller = move(poller), this ]() mutable {
      _pollers.emplace_back(move(poller));
    });
    return ret;
  }

  void shard::deregister_poller(usize poller_id) {
    dispatch([poller_id, this] {
      _pollers.erase(remove_if(begin(_pollers),
                               end(_pollers),
                               [&](auto const &poller) {
                                 return reinterpret_cast< usize >(
                                            address_of(poller)) == poller_id;
                               }),
                     end(_pollers));
    });
  }

  void shard::poll() {
    try {
      for (auto &&p : _pollers) {
        p->poll();
      }

      _task_queue.process_tasks();

      if (XI_UNLIKELY(!_inbound.tasks.empty())) {
        auto &tasks = _inbound.tasks;
        while (!tasks.empty()) {
          task *next_task = nullptr;
          if (tasks.pop(next_task) && next_task) {
            XI_SCOPE(exit) {
              delete next_task;
            };
            next_task->run();
          }
        }
      }
    } catch (...) {
      _handle_exception();
    }
  }

  void shard::_push_task_to_inbound_queue(own< task > t) {
    _inbound.tasks.push(t.release());
  }

  void shard::_post_task(own< task > t) {
    if (nullptr == this_shard || this != this_shard) {
      // local thread is not managed by the kernel
      // or is a remote thread
      // , so we must use a common input queue
      _push_task_to_inbound_queue(move(t));
    } else {
      // _task_queue.submit(move(t));
      _deferred_poller->push(move(t));
    }
  }

  void shard::_handle_exception() {
    _kernel->handle_exception(current_exception());
  }
}
}
