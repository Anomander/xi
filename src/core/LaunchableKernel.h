#pragma once

#include "core/Kernel.h"

namespace xi {
namespace core {

  template < class Launcher >
  class LaunchableKernel : public Kernel {
    vector< Launcher > _threads;

  public:
    void start(unsigned count, unsigned perCoreQueueSize) override {
      Kernel::start(count, perCoreQueueSize);
      for (unsigned t = 0; t < count; ++t) {
        _threads.emplace_back(machine().cpu(t));
        _threads[t].start([this, t] {
          _threads[t].pin();
          runOnCore(t);
        });
      }
    }
    void awaitShutdown() override {
      for (auto &t : _threads) {
        t.join();
      }
    }
  };

}
}
