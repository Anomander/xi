#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {
  class resumable;

  class abstract_coordinator {
  public:
    virtual void start(u8 cores)                 = 0;
    virtual void join()                          = 0;
    virtual void schedule(resumable* r)          = 0;
  };
}
}
