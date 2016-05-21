#pragma once

#include "xi/core/abstract_worker.h"
#include "xi/core/policy/access.h"
#include "xi/core/resumable.h"
#include "xi/core/runtime.h"

namespace xi {
namespace core {

  template < class Policy >
  class worker final : public policy::worker_access< Policy >,
                       public abstract_worker {
    thread _thread;
    resumable* _current_task;

  public:
    void attach_resumable(resumable* r) final override {
      r->attach_executor(this);
    }

    void detach_resumable(resumable* r) final override {
      r->detach_executor(this);
    }

    abstract_reactor* attach_pollable(resumable* r, i32 poll) override {
      return this->policy.attach_pollable(this, r, poll);
    }

    void detach_pollable(resumable* r, i32 poll) override {
      this->policy.detach_pollable(this, r, poll);
    }

    void schedule(resumable* r) override {
      if (&runtime.local_worker() == this) {
        this->policy.push_internally(this, r);
      } else {
        this->policy.push_externally(this, r);
      }
    }

    void schedule_locally(resumable* r) override {
      this->policy.push_internally(this, r);
    }

    void sleep_for(resumable* r, milliseconds ms) final override {
      this->policy.sleep_for(this, r, ms);
    }

    resumable* current_resumable() override {
      return _current_task;
    }

    void destroy(resumable* r) {
      detach_resumable(r);
      delete r;
    }

  public:
    void run() {
      for (;;) {
        _current_task = this->policy.pop(this);
        this->policy.begin_task(this, _current_task);

        auto result = _current_task->resume();

        this->policy.end_task(this, _current_task);

        switch (result) {
          case resumable::done:
            this->destroy(_current_task);
            break;
          case resumable::blocked:
            break;
          case resumable::resume_later:
            this->policy.push_internally(this, _current_task);
            break;
        }
        _current_task = nullptr;
      }
    }
  };
}
}
