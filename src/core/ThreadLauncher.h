#pragma once

#include "ext/Configure.h"
#include "hw/Hardware.h"

namespace xi {
namespace core {

  class ThreadLauncher {
    thread _thread;
    hw::Cpu const &_cpu;

  public:
    ThreadLauncher(hw::Cpu const &cpu) : _cpu(cpu) {}
    template < class F >
    void start(F &&f) {
      _thread = thread{forward< F >(f)};
    }

    void pin() const {
      cpu_set_t cs;
      CPU_ZERO(&cs);
      CPU_SET(_cpu.id().os, &cs);
      auto r = pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
      assert(r == 0);
    }

    void join() { _thread.join(); }
  };

}
}
