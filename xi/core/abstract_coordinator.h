#pragma once

#include "xi/ext/configure.h"
#include "xi/core/execution_context.h"

namespace xi {
namespace core {
  class abstract_coordinator : public execution_context {
  public:
    virtual void start(u8 cores) = 0;
    virtual void join() = 0;
    virtual resumable* current_thread_resumable() = 0;
    virtual execution_context* current_thread_worker() = 0;
  };

}
}
