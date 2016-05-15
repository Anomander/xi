#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {

  enum event_state : u16 {
    kNone      = 0,
    kRead      = 1,
    kWrite     = 1 << 1,
    kTimeout   = 1 << 2,
    kClose     = 1 << 3,
    kError     = 1 << 4,
    kReadWrite = kRead | kWrite,
  };

  class event : virtual public ownership::unique {
  public:
    virtual ~event() noexcept              = default;
    virtual void cancel()                  = 0;
    virtual void arm()                     = 0;
    virtual void change_state(event_state) = 0;
    virtual void add_state(event_state)    = 0;
    virtual void remove_state(event_state) = 0;
    virtual bool is_active()               = 0;
  };
}
}
