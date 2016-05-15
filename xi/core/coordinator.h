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
      printf("Calling blocking lambda\n");
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

    execution_context* current_thread_worker() override {
      return this->policy.current_worker(this);
    }

    resumable* current_thread_resumable() override {
      auto w = this->policy.current_worker(this);
      // printf("Worker for thread %p is %p.\n", pthread_self(), w);
      assert(w != nullptr);
      return w->current_resumable();
    }

    void attach_resumable(resumable* r) override {
      // assert(COORDINATOR == this);
      auto w = this->policy.current_worker(this);
      assert(w != nullptr);
      w->attach_resumable(r);
    }

    void detach_resumable(resumable* r) override {
      // assert(COORDINATOR == this);
      auto w = this->policy.current_worker(this);
      assert(w != nullptr);
      w->detach_resumable(r);
    }

    abstract_reactor* attach_pollable(resumable* r, i32 poll) override {
      // assert(COORDINATOR == this);
      auto w = this->policy.current_worker(this);
      assert(w != nullptr);
      return w->attach_pollable(r, poll);
    }

    void detach_pollable(resumable* r, i32 poll) override {
      // assert(COORDINATOR == this);
      auto w = this->policy.current_worker(this);
      assert(w != nullptr);
      w->detach_pollable(r, poll);
    }
    void schedule(resumable* r) override {
      // assert(COORDINATOR == this);
      this->policy.central_schedule(this, r);
    }
    void sleep_for(resumable* r, milliseconds ms) override {
      // assert(COORDINATOR == this);
      auto w = this->policy.current_worker(this);
      assert(w != nullptr);
      w->sleep_for(r, ms);
    }
  };
}
}
