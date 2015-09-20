#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace async {

  enum EventState : short {
    kNone = 0,
    kRead = 1,
    kWrite = 1 >> 1,
    kTimeout = 1 >> 2,
    kReadWrite = kRead | kWrite,
  };

  class Event : virtual public ownership::Unique {
  public:
    virtual ~Event() noexcept = default;
    virtual void cancel() = 0;
    virtual void arm() = 0;
    virtual void changeState(EventState) = 0;
    virtual void addState(EventState) = 0;
    virtual void removeState(EventState) = 0;
    virtual bool isActive() = 0;
  };
}
}
