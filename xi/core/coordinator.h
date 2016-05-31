#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/barrier.h"
#include "xi/core/abstract_coordinator.h"
#include "xi/core/execution_context.h"
#include "xi/core/policy/access.h"
#include "xi/core/worker.h"

namespace xi {
namespace core {

  class blocking_resumable : public resumable {
  public:
    virtual void call() = 0;

  private:
    resume_result resume() final override {
      call();
      return done;
    }

    void yield(resume_result) final override {
    }
  };

  template < class F >
  class lambda_blocking_resumable final : public blocking_resumable {
    F _f;

  public:
    lambda_blocking_resumable(F&& f) : _f(forward< F >(f)) {
    }

    void call() final override {
      _f();
    }
  };

  template < class F >
  auto make_lambda_blocking_resumable(F&& f) {
    return new lambda_blocking_resumable< decay_t< F > >(forward< F >(f));
  }

  template < class Policy >
  class coordinator : public policy::coordinator_access< Policy >,
                      public abstract_coordinator {
    using worker_t = worker< Policy >;

    vector< thread > _threads;
    unique_ptr<barrier> _barrier;

    static void pin(u8 cpu) {
      cpu_set_t cs;
      CPU_ZERO(&cs);
      CPU_SET(cpu, &cs);
      auto r = pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
      assert(r == 0);
    }

  public:
    void start(u8 cores) override {
      _barrier = make_unique<barrier>(cores + 1);
      for (auto cpu : range::to(cores)) {
        _threads.emplace_back([this, cpu] {
          auto w = this->policy.start_worker(this);
          runtime.current_worker = w;
          // printf("Pinning thread %p to core %d.\n", pthread_self(), cpu);
          pin(cpu);
          _barrier->wait();
          w->run();
        });
      }
      _barrier->wait();
    }

    void join() override {
      for (auto&& t : _threads) {
        if (t.joinable()) {
          t.join();
        }
      }
    }

    void schedule(resumable* r) override {
      // assert(COORDINATOR == this);
      this->policy.central_schedule(this, r);
    }
  };
}
}
