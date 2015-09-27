#pragma once

#include "xi/core/kernel.h"

namespace xi {
namespace core {

  template < class launcher > class launchable_kernel : public kernel {
    vector< launcher > _threads;

  public:
    void start(unsigned count, unsigned per_core_queue_size) override {
      kernel::start(count, per_core_queue_size);
      for (unsigned t = 0; t < count; ++t) {
        _threads.emplace_back(machine().core(t));
        _threads[t].start([this, t] {
          _threads[t].pin();
          run_on_core(t);
        });
      }
    }
    void await_shutdown() override {
      for (auto &t : _threads) { t.join(); }
      kernel::await_shutdown();
    }
  };
}
}
