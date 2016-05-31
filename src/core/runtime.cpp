#include "xi/core/runtime.h"
#include "xi/ext/once.h"
#include "xi/core/coordinator.h"
#include "xi/core/policy/worker_isolation.h"
#include "xi/core/scheduler.h"
#include "xi/core/shared_queue.h"

namespace xi {
namespace core {

  namespace v2 {
    class runtime_environment::impl {
      vector< own< scheduler > > _schedulers;
      atomic< u64 > _round_robin_cnt{0};
      u8 _cores;

    public:
      void start();
      void await_termination();
      void spawn(own< resumable_builder > rb);
      void set_cores(u8);
      void sleep_current_resumable(nanoseconds ns);
      void await_readable(i32 fd);
      void await_writable(i32 fd);
    };

    void runtime_environment::impl::start() {
      _schedulers.push_back(make< scheduler >());
      // TODO : Integrate with libnuma and create a scheduler per node
      for (auto&& s : _schedulers) {
        s->start(0, _cores);
      }
    }

    void runtime_environment::impl::await_termination() {
      for (auto&& s : _schedulers) {
        s->join();
      }
    }

    void runtime_environment::impl::spawn(own< resumable_builder > rb) {
      auto idx = _round_robin_cnt.fetch_add(1, memory_order_relaxed) %
                 _schedulers.size();
      _schedulers[idx]->central_enqueue(move(rb));
    }

    void runtime_environment::impl::set_cores(u8 count) {
      _cores = count;
    }

    void runtime_environment::impl::sleep_current_resumable(nanoseconds ns) {
      auto w = LOCAL_WORKER;
      assert(is_valid(w));
      return w->sleep_current(ns);
    }

    void runtime_environment::impl::await_readable(i32 fd) {
      auto w = LOCAL_WORKER;
      assert(is_valid(w));
      return w->await_readable(fd);
    }

    void runtime_environment::impl::await_writable(i32 fd) {
      auto w = LOCAL_WORKER;
      assert(is_valid(w));
      return w->await_writable(fd);
    }

    runtime_environment::runtime_environment() : _impl(make_shared< impl >()) {
    }

    runtime_environment::~runtime_environment() {
      if (is_valid(_impl)) {
        _impl->await_termination();
      }
    }

    void runtime_environment::spawn(own< resumable_builder > rb) {
      once::call_lambda([this] { _impl->start(); });
      _impl->spawn(move(rb));
    }

    void runtime_environment::set_cores(u8 count) {
      assert(is_valid(_impl));
      return _impl->set_cores(count);
    }

    void runtime_environment::sleep_current_resumable(nanoseconds ns) {
      assert(is_valid(_impl));
      return _impl->sleep_current_resumable(ns);
    }

    void runtime_environment::await_readable(i32 fd) {
      assert(is_valid(_impl));
      return _impl->await_readable(fd);
    }

    void runtime_environment::await_writable(i32 fd) {
      assert(is_valid(_impl));
      return _impl->await_writable(fd);
    }

    runtime_environment runtime;
  }

  thread_local struct worker_stats WORKER_STATS;

  namespace {
    static abstract_coordinator* COORDINATOR = nullptr;
  }

  thread_local abstract_worker* runtime_context::current_worker = 0;
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

  abstract_worker& runtime_context::local_worker() {
    return *current_worker;
  }
}
}
