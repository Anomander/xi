#pragma once

#include "xi/ext/configure.h"
#include "xi/core/abstract_coordinator.h"
#include "xi/core/abstract_worker.h"
#include "xi/core/resumable_builder.h"

namespace xi {
namespace core {
  namespace v2 {
    class runtime_environment {
      class impl;
      shared_ptr< impl > _impl;

    public:
      runtime_environment();
      ~runtime_environment();
      void spawn(own<resumable_builder>);
      void set_cores(u8);

      void sleep_current_resumable(nanoseconds);
      void await_readable(i32);
      void await_writable(i32);
    };

    extern runtime_environment runtime;
  }

  class runtime_context {
    class coordinator_builder;

  public:
    runtime_context();
    ~runtime_context();

    template < class Policy >
    static void set_work_policy();

    void set_max_cores(u8);

    abstract_coordinator& coordinator();
    abstract_worker& local_worker();

    thread_local static abstract_worker* current_worker;

  private:
    unique_ptr< coordinator_builder > _coordinator_builder;
  };

  extern runtime_context runtime;
}
}
