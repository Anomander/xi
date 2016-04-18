#include "xi/core/shard.h"
#include "xi/core/bootstrap.h"
#include "xi/core/epoll/reactor.h"
#include "xi/core/message_bus.h"
#include "xi/core/message_bus_impl.h"
#include "xi/core/signals.h"

namespace xi {
mut< core::shard >
shard_at(u16 id) {
  return core::bootstrap::shard_at(id);
}

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
    mut< core::reactor > _reactor;

  public:
    reactor_poller(mut< core::reactor > reactor) : _reactor(reactor) {
    }
    unsigned poll() noexcept override {
      try {
        _reactor->poll();
      } catch (...) {
      }
      return 0;
    }
  };

  class message_bus_poller : public poller {
    u8 _cpu;
    message_bus **_buses;

  public:
    message_bus_poller(u8 cpu, message_bus **mb) : _cpu(cpu), _buses(mb) {
      std::cout << "Buses: " << _buses << std::endl;
    }

    unsigned poll() noexcept override {
      for (auto i : range::to(bootstrap::cpus())) {
        if (i != _cpu) {
          _buses[i][_cpu].process_pending();
          _buses[_cpu][i].process_complete();
        }
      }
      return 0;
    }
  };

  thread_local mut< shard > this_shard = nullptr;

  shard::shard(u8 core, message_bus **mb) : _core_id(core), _buses(mb) {
    std::cout << "Starting shard @" << _core_id << " in thread "
              << pthread_self() << std::endl;
    std::cout << "Buses: " << _buses << std::endl;
    _reactor = make< epoll::reactor >(this);
  }

  void shard::init() {
    register_poller(make< reactor_poller >(edit(_reactor)));

    auto sig = make< signals >();
    _signals = edit(sig);
    register_poller(move(sig));

    auto dp          = make< deferred_poller >(edit(_task_queue));
    _deferred_poller = edit(dp);
    register_poller(move(dp));

    // if (bootstrap::cpus() > 1 && _buses) {
    //   register_poller(make< message_bus_poller >(_core_id, _buses));
    // run
  }

  void shard::run() {
    std::cout << "Shard @" << _core_id << " is running." << std::endl;
    _running = true;
    while (true) {
      poll();
      if (XI_UNLIKELY(!_running)) {
        break;
      }
    }
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
                               [poller_id](auto const &poller) {
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
        _inbound.tasks.consume_all([](task *t) {
          XI_SCOPE(exit) {
            delete t;
          };
          t->run();
        });
      }
    } catch (...) {
      _handle_exception();
    }
  }

  void shard::shutdown() {
    _running = false;
  }

  void shard::_push_task_to_inbound_queue(u16 remote_cpu, own< task > t) {
    _inbound.tasks.push(t.release());
    // _buses[cpu()][remote_cpu].submit([t = move(t)] { t->run(); });
  }

  void shard::_post_task(own< task > t) {
    assert(this_shard);
    if (this != this_shard) {
      // local thread is not managed by the kernel
      // or is a remote thread
      // , so we must use a common input queue
      _push_task_to_inbound_queue(this_shard->cpu(), move(t));
    } else {
      // _task_queue.submit(move(t));
      _deferred_poller->push(move(t));
    }
  }

  void shard::_handle_exception() {
    bootstrap::initiate_shutdown();
  }
}
}
