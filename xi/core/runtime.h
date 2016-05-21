#pragma once

#include "xi/ext/configure.h"
#include "xi/core/abstract_coordinator.h"
#include "xi/core/abstract_worker.h"

namespace xi {
namespace core {
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
    unique_ptr<coordinator_builder> _coordinator_builder;
  };

  extern runtime_context runtime;
}
}
