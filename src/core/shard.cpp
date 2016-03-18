#include "xi/core/shard.h"
#include "xi/core/kernel.h"

namespace xi {
namespace core {

  thread_local mut< shard > this_shard = nullptr;

  shard::shard(mut< kernel > k, u16 core, queues_t &qs)
      : _core_id(core), _queues(qs), _kernel(k) {
  }

  usize shard::register_poller(own< poller > poller) {
    auto ret = reinterpret_cast< size_t >(address_of(poller));
    post([ poller = move(poller), this ]() mutable {
      _pollers.emplace_back(move(poller));
    });
    return ret;
  }

  void shard::deregister_poller(usize poller_id) {
    dispatch([poller_id, this] {
      _pollers.erase(
          remove_if(begin(_pollers), end(_pollers), [&](auto const &poller) {
            return reinterpret_cast< usize >(address_of(poller)) == poller_id;
          }), end(_pollers));
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
        auto lock = make_lock(_inbound.lock);
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
