#include "xi/core/shard.h"
#include "xi/core/kernel.h"

namespace xi {
namespace core {

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

  shard::shard(mut< kernel > k, u16 core, queues_t &qs)
      : _core_id(core), _queues(qs), _kernel(k) {
    std::cout << "Starting shard @" << _core_id << " in thread "
              << pthread_self() << std::endl;
  }

  void shard::attach_reactor(own< core::reactor > r) {
    _reactor = edit(r);
    register_poller(make< reactor_poller >(move(r)));
  }

  usize shard::register_poller(own< poller > poller) {
    auto ret = reinterpret_cast< size_t >(address_of(poller));
    post([ poller = move(poller), this ]() mutable {
      _pollers.emplace_back(move(poller));
    });
    return ret;
  }

  void shard::deregister_poller(usize poller_id) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
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
      for (auto &&q : _queues[_core_id]) {
        q->process_tasks();
      }

      for (auto &&p : _pollers) {
        p->poll();
      }

      if (XI_UNLIKELY(!_inbound.tasks.empty())) {
        auto lock   = make_lock(_inbound.lock);
        auto &tasks = _inbound.tasks;
        while (!tasks.empty()) {
          auto &next_task = tasks.back();
          XI_SCOPE(exit) {
            tasks.pop();
          }; // pop no matter what, we don't want to retry throwing tasks
          next_task->run();
        }
      }
    } catch (...) {
      _handle_exception();
    }
  }

  void shard::_handle_exception() {
    _kernel->handle_exception(current_exception());
  }
}
}
