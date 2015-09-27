#pragma once

#include "xi/ext/configure.h"
#include "xi/hw/hardware.h"

namespace xi {
namespace core {

  class thread_launcher {
    thread _thread;
    hw::cpu const &_cpu;

  public:
    thread_launcher(hw::cpu const &cpu) : _cpu(cpu) {}
    template < class F > void start(F &&f) {
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
