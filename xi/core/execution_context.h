#pragma once

#include "xi/ext/configure.h"
#include "xi/core/policy/access.h"

namespace xi {
namespace core {

  class resumable;
  class abstract_reactor;

  class execution_context {
  public:
    virtual ~execution_context() = default;
    virtual void attach_resumable(resumable*) = 0;
    virtual void detach_resumable(resumable*) = 0;
    virtual abstract_reactor* attach_pollable(resumable*, i32 poll) = 0;
    virtual void detach_pollable(resumable*, i32 poll) = 0;
    virtual void schedule(resumable*) = 0;
    virtual void sleep_for(resumable*, milliseconds) = 0;
  };

  // extern static thread_local execution_context* COORDINATOR;
  extern thread_local execution_context* WORKER;
}
}
