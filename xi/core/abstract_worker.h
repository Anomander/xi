#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {
  class abstract_reactor;
  class resumable;

  class abstract_worker {
  public:
    virtual ~abstract_worker() = default;
    virtual resumable* current_resumable() = 0;
    virtual void attach_resumable(resumable*) = 0;
    virtual void detach_resumable(resumable*) = 0;
    virtual abstract_reactor* attach_pollable(resumable*, i32 poll) = 0;
    virtual void detach_pollable(resumable*, i32 poll) = 0;
    virtual void schedule(resumable*) = 0;
    virtual void schedule_locally(resumable*) = 0;
    virtual void sleep_for(resumable*, milliseconds) = 0;

  public:
    template < class F,
               class... Args,
               XI_REQUIRE_DECL(is_base_of< resumable, F >) >
    void spawn_resumable(Args && ... args) {
      auto r = new F(forward<Args>(args)...);
      attach_resumable(r);
      schedule_locally(r);
    }

  };
}
}
