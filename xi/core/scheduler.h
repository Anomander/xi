#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/barrier.h"
#include "xi/core/netpoller.h"
#include "xi/core/resumable.h"
#include "xi/core/shared_queue.h"
#include "xi/core/worker2.h"

namespace xi {
namespace core {
  namespace v2 {
    namespace {
      static void pin(u8 cpu) {
        cpu_set_t cs;
        CPU_ZERO(&cs);
        CPU_SET(cpu, &cs);
        auto r = pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
        assert(r == 0);
      }
    }

    class scheduler : public virtual ownership::unique {
      enum class worker_state {
        RUNNING,
        PARKED_NETPOLL,
        PARKED_THREAD,
        SPINNING
      };

      struct worker_control_block {
        mut< worker > w;
        mut< netpoller > poller;
        own< shared_queue > input_queue;
        /// These two queues should only be mutated when the worker requests it.
        mut< worker_queue > ready_queue;
        mut< worker_queue > port_queue;
        worker::config worker_config;
        unique_ptr< mutex > worker_mutex;
        unique_ptr< condition_variable > worker_cv;
        worker_state state;
      };

      unique_ptr< barrier > _barrier;
      vector< thread > _threads;

      own< netpoller > _netpoller;

      alignas(64) vector< worker_control_block > _workers;
      atomic< u64 > _parked_workers{0};
      atomic< u64 > _active_workers{0};

    public:
      void start(u16 core_start, u16 core_end); // TODO: Change to range
      void join();
      void central_enqueue(own< resumable_builder >);
      void idle_worker(mut< worker >);
      void central_sleep(own< resumable >, steady_clock::time_point);

    private:
      void _park(mut< worker >);
      mut< worker_control_block > _worker_for_job(ref< resumable_builder >);
      opt< mut< worker_control_block > > _first_parked_worker();
      mut< worker_control_block > _least_loaded_worker();
    };

    inline void scheduler::start(u16 core_start, u16 core_end) {
      assert(core_end >= core_start);
      auto cores = core_end - core_start;
      _barrier   = make_unique< barrier >(cores + 1);

      _workers.resize(cores);

      for (auto cpu : range::to(cores)) {
        _threads.emplace_back([ this, cpu, idx = cpu - core_start ] {
          auto worker_queue = make< shared_queue >();

          auto poller = make< netpoller >();
          poller->start();

          worker w(edit(poller), edit(worker_queue), this, idx);
          _workers[idx] = {
              // mut<worker> w;
              edit(w),
              // mut< netpoller > poller;
              edit(poller),
              // own< shared_queue > input_queue;
              move(worker_queue),
              // mut< worker_queue > ready_queue;
              w.ready_queue(),
              // mut< worker_queue > port_queue;
              w.port_queue(),
              // worker::config worker_config;
              worker::DEFAULT_CONFIG,
              // unique_ptr<mutex> worker_mutex;
              make_unique< mutex >(),
              // unique_ptr<condition_variable> worker_cv;
              make_unique< condition_variable >(),
              // worker_state state;
              worker_state::RUNNING,
          };
          LOCAL_WORKER = edit(w);
          // printf("Pinning thread %p to core %d.\n", pthread_self(), cpu);
          pin(cpu);
          _active_workers.fetch_or(1ul << idx, memory_order_release);
          _barrier->wait();
          w.run(); // TODO: Handle exceptions

          // TODO: clean up
          _workers[idx].w = nullptr;
        });
      }
      _barrier->wait();
    }

    inline void scheduler::join() {
      for (auto&& t : _threads) {
        if (t.joinable()) {
          t.join();
        }
      }
    }

    inline void scheduler::central_enqueue(own< resumable_builder > rb) {
      // printf("%s\n", __PRETTY_FUNCTION__);
      /// Unpark a worker and schedule this work on it
      auto w = _worker_for_job(val(rb));
      w->input_queue->enqueue(move(rb));
      // printf("The state is %lu.\n", (usize)w->state);
      if (w->state == worker_state::PARKED_NETPOLL) {
        // printf("Unparking netpoll.\n");
        w->poller->unblock_one();
      }
      // } else if (w->state == worker_state::PARKED_THREAD) {
      //   {
      //     auto lock = make_unique_lock(*w->worker_mutex);
      //     w->state  = worker_state::RUNNING;
      //   }
      //   w->worker_cv->notify_one();
      // }
    }

    inline void scheduler::central_sleep(own< resumable > r,
                                         steady_clock::time_point t) {
      // return _local_sleep_queue.enqueue(t, move(r));
    }

    /// The worker has run out of any possible work.
    /// Either steal work from other workers, or park if
    /// none available.
    inline void scheduler::idle_worker(mut< worker > w) {
      // TODO: Add work stealing and return
      _park(w);
    }

    inline void scheduler::_park(mut< worker > w) {
      auto idx     = w->index();
      auto& w_ctrl = _workers[idx];

      auto park_idx = 1ul << idx;
      /// If this is the last worker running, then park
      /// it in netpoller, otherwise park its thread.
      // printf("%p parking in netpoll.\n", pthread_self());
      _parked_workers.fetch_or(park_idx, memory_order_acq_rel);
      w_ctrl.state = worker_state::PARKED_NETPOLL;
      w_ctrl.poller->blocking_poll_into(w_ctrl.port_queue, 1);
      // if (park_idx ==
      //     _active_workers.fetch_and(~park_idx, memory_order_acq_rel)) {
      //   /// This is the last non-parked worker
      //   w_ctrl.state = worker_state::PARKED_NETPOLL;
      //   printf("%p parking in netpoll.\n", pthread_self());
      //   _netpoller->blocking_poll_into(w_ctrl.port_queue, 1);
      // } else {
      //   w_ctrl.state = worker_state::PARKED_THREAD;
      //   auto lock    = make_unique_lock(*w_ctrl.worker_mutex);
      //   printf("%p parking in thread.\n", pthread_self());
      //   w_ctrl.worker_cv->wait(lock);
      // }
      w_ctrl.state = worker_state::RUNNING;
      // _active_workers.fetch_or(park_idx, memory_order_release);
      _parked_workers.fetch_and(~park_idx, memory_order_release);
      // printf("%p un-parking.\n", pthread_self());
    }

    inline auto scheduler::_worker_for_job(ref< resumable_builder >)
        -> mut< worker_control_block > {
      assert(_workers.size() > 0);
      // TODO: Add worker affinity

      /// For unaffined processes pick the best worker.
      /// The selection logic is as follows: (1) if any number of
      /// workers is parked, unpark one and use it, or (2) if no
      /// workers are parked, use the worker with least amount of
      /// work last reported.
      return _first_parked_worker().unwrap_or(
          [this] { return _least_loaded_worker(); });
    }

    inline auto scheduler::_first_parked_worker()
        -> opt< mut< worker_control_block > > {
      while (auto parked = _parked_workers.load(memory_order_acquire)) {
        usize idx = count_trailing_zeroes(parked);
        if (parked !=
            _parked_workers.fetch_and(~(1ul << idx), memory_order_release)) {
          /// Lost a race, need to start over
          continue;
        }
        assert(idx < _workers.size());
        // printf("First parked worker is %lu.\n", idx);
        // printf("Its state is %lu.\n", (usize)_workers[idx].state);
        return some(edit(_workers[idx]));
      }
      // printf("No parked workers.\n");
      return none;
    }

    inline auto scheduler::_least_loaded_worker()
        -> mut< worker_control_block > {
      assert(_workers.size() > 0);
      // TODO: This is round robin for now, replace with actual load factors
      thread_local static u64 ROUND_ROBIN = 0;
      return edit(_workers[ROUND_ROBIN++ % _workers.size()]);
    }
  }
}
}
