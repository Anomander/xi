#pragma once

#include "xi/async/latch.h"
#include "xi/core/kernel.h"

namespace xi {
namespace core {

  template < class L, class R >
  class launchable_kernel : public kernel {
    vector< L > _threads;
    shared_ptr< async::promise<> > _shutdown_promise;
    shared_ptr< async::latch > _start_latch;

  public:
    async::future<> start(unsigned count,
                          unsigned per_core_queue_size) override {
      _start_latch      = make_shared< async::latch >(count);
      _shutdown_promise = make_shared< async::promise<> >();
      kernel::start(count, per_core_queue_size).then([this, count]() mutable {
        for (unsigned t = 0; t < count; ++t) {
          _threads.emplace_back(machine().core(t));
          _threads[t].start([this, t] {
            _threads[t].pin();
            run_on_core(t);
          });
        }
      });
      return _start_latch->await();
    }

    void initiate_shutdown() override {
      if (_shutdown_promise) {
        _shutdown_promise->set();
      }
      kernel::initiate_shutdown();
    }

    void await_shutdown() override {
      for (auto &t : _threads) {
        t.join();
      }
      kernel::await_shutdown();
    }

    void startup(u16 id) override {
      kernel::startup(id);
      _start_latch->count_down();
    }

    mut<shard> make_shard(u16 id) override {
      auto s = kernel::make_shard(id);
      auto r = make<R>(s);
      s->attach_reactor(move(r));
      return s;
    }

    auto before_shutdown() {
      if (!_shutdown_promise) {
        throw std::logic_error("Kernel has not been started.");
      }
      return _shutdown_promise->get_future();
    }
  };
}
}
