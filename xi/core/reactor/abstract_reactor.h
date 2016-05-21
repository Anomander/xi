#pragma once

#include "xi/ext/configure.h"
#include "xi/core/reactor/abstract_reactor.h"

namespace xi {
namespace core {
  class resumable;
  class abstract_worker;

  class abstract_reactor {
  public:
    XI_CLASS_DEFAULTS(abstract_reactor, virtual_dtor);

    virtual void poll_for(nanoseconds) = 0;
    virtual void attach_pollable(resumable*, i32) = 0;
    virtual void detach_pollable(resumable*, i32) = 0;
    virtual void await_signal(resumable*, i32)    = 0;
    virtual void await_readable(resumable*, i32)  = 0;
    virtual void await_writable(resumable*, i32)  = 0;
  };
}
}
