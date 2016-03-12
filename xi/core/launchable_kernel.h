#pragma once

#include "xi/core/kernel.h"
#include "xi/async/latch.h"

namespace xi {
namespace core {

  template < class launcher > class launchable_kernel : public kernel {
    vector< launcher > _threads;
    shared_ptr< async::promise<> > _shutdown_promise;

  public:
    void start(unsigned count, unsigned per_core_queue_size) override {
      _shutdown_promise = make_shared< async::promise<> >();
      kernel::start(count, per_core_queue_size);
      for (unsigned t = 0; t < count; ++t) {
        _threads.emplace_back(machine().core(t));
        _threads[t].start([this, t] {
          _threads[t].pin();
          run_on_core(t);
        });
      }
    }

    void initiate_shutdown() override {
      if (_shutdown_promise) {
        _shutdown_promise->set();
      }
      kernel::initiate_shutdown();
    }

    void await_shutdown() override {
      for (auto &t : _threads) { t.join(); }
      kernel::await_shutdown();
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
