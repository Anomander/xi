#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {
  class abstract_reactor {
  public:
    XI_CLASS_DEFAULTS(abstract_reactor, virtual_dtor);

    virtual void poll_for(nanoseconds) = 0;
    virtual void await_readable(i32) = 0;
    virtual void await_writable(i32) = 0;
  };
}
}
