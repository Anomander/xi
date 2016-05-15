#include "xi/core/runtime.h"
#include "xi/ext/once.h"
#include "xi/core/coordinator.h"
#include "xi/core/policy/worker_isolation.h"

namespace xi {
namespace core {
  namespace {
    static abstract_coordinator* COORDINATOR = nullptr;
  }

  runtime_context runtime;

  class runtime_context::coordinator_builder {
    struct abstract_maker {
      virtual abstract_coordinator* make() = 0;
    };

    u8 _n_cores            = 1;
    abstract_maker* _maker = set_work_policy< policy::worker_isolation >();

  public:
    template < class Policy >
    abstract_maker* set_work_policy() {
      static struct concrete_maker : abstract_maker {
        abstract_coordinator* make() override {
          return new xi::core::coordinator< Policy >;
        }
      } maker;
      _maker = &maker;
      return _maker;
    }

    void set_max_cores(u8 cores) {
      _n_cores = cores;
    }

    abstract_coordinator* build() {
      assert(_maker != nullptr);
      auto c = _maker->make();
      c->start(_n_cores);
      return c;
    }
  };

  runtime_context::runtime_context()
      : _coordinator_builder(new coordinator_builder) {
  }

  runtime_context::~runtime_context() {
    if (COORDINATOR) {
      COORDINATOR->join();
    }
  }

  void runtime_context::set_max_cores(u8 cores) {
    _coordinator_builder->set_max_cores(cores);
  }

  abstract_coordinator& runtime_context::coordinator() {
    once::call_lambda(
        [] { COORDINATOR = runtime._coordinator_builder->build(); });
    return *COORDINATOR;
  }
}
}
