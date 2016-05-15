#pragma once

#include "xi/ext/configure.h"
#include "xi/core/detail/intrusive.h"
#include "xi/core/reactor/abstract_reactor.h"

namespace xi {
namespace core {
  class resumable;
  class execution_context;

  class abstract_reactor {
    execution_context* _executor = nullptr;

  public:
    XI_CLASS_DEFAULTS(abstract_reactor, virtual_dtor);

    void attach_executor(execution_context* e) {
      _executor = e;
    }

    void detach_executor(execution_context*) {
      _executor = nullptr;
    }

    virtual void poll_for(milliseconds) = 0;
    virtual void attach_pollable(resumable*, i32) = 0;
    virtual void detach_pollable(resumable*, i32) = 0;
    virtual void await_signal(resumable*, i32)    = 0;
    virtual void await_readable(resumable*, i32)  = 0;
    virtual void await_writable(resumable*, i32)  = 0;

  protected:
    execution_context* executor() const {
      return _executor;
    }
  };
}
}
