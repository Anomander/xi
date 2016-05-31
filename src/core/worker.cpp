#include "xi/core/netpoller.h"
#include "xi/core/runtime.h"
#include "xi/core/scheduler.h"
#include "xi/core/shared_queue.h"
#include "xi/core/sleep_queue.h"
#include "xi/core/worker2.h"

namespace xi {
namespace core {
  namespace v2 {
    enum {
      DEFAULT_NETPOLL_MAX                = numeric_limits< usize >::max(),
      DEFAULT_SCHEDULER_DEQUEUE_MAX      = 100,
      DEFAULT_LOOP_BUDGET_ALLOCATION     = 1'000'000,
      DEFAULT_MAX_LOOP_BUDGET_ALLOCATION = 5'000'000,
      DEFAULT_ISOL_BUDGET_ALLOCATION     = 10'000'000,
      DEFAULT_SPINS_BEFORE_IDLE          = 1000,
      UPPER_BOUND_READY_QUEUE_NS         = 5'000'0,
      UPPER_BOUND_FAST_QUEUE_NS          = 100'000'000,
    };

    thread_local mut< worker > LOCAL_WORKER = nullptr;
    worker::config worker::DEFAULT_CONFIG   = {
        // usize netpoll_max;
        DEFAULT_NETPOLL_MAX,
        // usize scheduler_dequeue_max;
        DEFAULT_SCHEDULER_DEQUEUE_MAX,
        // usize loop_budget_allocation;
        DEFAULT_LOOP_BUDGET_ALLOCATION,
        // usize max_loop_budget_allocation;
        DEFAULT_MAX_LOOP_BUDGET_ALLOCATION,
        // usize isolation_budget_allocation;
        DEFAULT_ISOL_BUDGET_ALLOCATION,
        // usize nr_spins_before_idle;
        DEFAULT_SPINS_BEFORE_IDLE,
        // timer_bounds;
        {
            // nanoseconds upper_bound_ready_queue;
            nanoseconds(UPPER_BOUND_READY_QUEUE_NS),
            // nanoseconds upper_bound_fast_queue;
            nanoseconds(UPPER_BOUND_FAST_QUEUE_NS),
        }};

    worker::worker(mut< netpoller > n,
                   mut< shared_queue > sq,
                   mut< scheduler > s,
                   u64 i,
                   config c)
        : _netpoller(n)
        , _scheduler_queue(sq)
        , _scheduler(s)
        , _index(i)
        , _config(move(c)) {
    }

    void worker::run() {
      usize loop_budget_allocation = _config.loop_budget_allocation;
      u64 spins                    = 0;
    central_schedule:
      for (;;) {
        auto cnt = _scheduler_queue->dequeue_into(
            edit(_ready_queue), _config.scheduler_dequeue_max);

        /// Decide whether to report as idle to scheduler
        if (!cnt // scheduler queue has nothing
            &&
            _local_sleep_queue.is_empty() // short sleep queue has nothing
            &&
            ++spins > _config.nr_spins_before_idle // spun for enough rounds
            ) {
          /// Report as idle. Scheduler will block until it has more work.
          spins = 0;
          _scheduler->idle_worker(this);
          goto central_schedule;
        }
        /// This budget governs how long a worker can stay inside the hot
        /// loop without going to scheduler for external work
        execution_budget isol_budget(_config.isolation_budget_allocation);
      poll:
        for (; isol_budget.adjust_spent();) {
          /// Get work from netpoller
          _netpoller->poll_into(edit(_port_queue), _config.netpoll_max);

          /// Check short sleep queue for expired items
          _local_sleep_queue.dequeue_into(edit(_ready_queue),
                                          steady_clock::now());

          /// This budget governs how long a worker can be processing
          /// ready event without getting work from netpoller
          execution_budget loop_budget(loop_budget_allocation);

          /// Reset spin count if there's work to be done
          if (!_port_queue.is_empty() || !_ready_queue.is_empty()) {
            spins = 0;
          }

          /// Ports are processed first and may starve non-ports of
          /// budget
          while (!_port_queue.is_empty()) {
            spins = 0;
            _run_task_from_queue(edit(_port_queue), edit(loop_budget));
            if (XI_UNLIKELY(!loop_budget.adjust_spent())) {
              goto poll;
            }
          }

          /// Non-ports are given lower priority (even at the same priority
          /// level) to reduce externally observable latency.
          while (!_ready_queue.is_empty()) {
            _run_task_from_queue(edit(_ready_queue), edit(loop_budget));
            if (XI_UNLIKELY(!loop_budget.adjust_spent())) {
              goto poll;
            }
          }

          /// Carry over unspent budget into the next round
          if (!loop_budget.is_expended()) {
            loop_budget_allocation =
                max(_config.max_loop_budget_allocation,
                    loop_budget_allocation + loop_budget.remainder());
            goto central_schedule;
          }
          loop_budget_allocation = _config.loop_budget_allocation;
        }
      }
    }

    void worker::_block_resumable_on_sleep(own< resumable > r, nanoseconds ns) {
      if (ns < _config.timer_bounds.upper_bound_ready_queue) {
        return _ready_queue.enqueue(move(r));
      }
      // TODO: add support for long queues
      // if (ns > _config.timer_bounds.upper_bound_fast_queue) {
      //   return _scheduler->central_sleep(move(r), steady_clock::now() + ns);
      // }
      return _local_sleep_queue.enqueue(steady_clock::now() + ns, move(r));
    }

    void worker::_run_task_from_queue(mut< worker_queue > q,
                                      mut< execution_budget > budget) {
      _current_task = q->dequeue().unwrap();

      auto r_start = high_resolution_clock::now();
      auto result  = _current_task->resume(budget);
      struct match : resumable::match {
        mutable mut< worker > _worker;
        mutable own< resumable > _resumable;

        match(mut< worker > w, own< resumable > r)
            : _worker(w), _resumable(move(r)) {
        }

        void operator()(resumable::done) const {
        }
        void operator()(resumable::reschedule) const {
          _worker->_ready_queue.enqueue(move(_resumable));
        }
        void operator()(resumable::blocked::sleep sleep) const {
          _worker->_block_resumable_on_sleep(move(_resumable), sleep.duration);
        }
        void operator()(resumable::blocked::port port) const {
          switch (port.type) {
            case resumable::blocked::port::READ:
              return _worker->_netpoller->await_readable(port.fd,
                                                         move(_resumable));
            case resumable::blocked::port::WRITE:
              return _worker->_netpoller->await_writable(port.fd,
                                                         move(_resumable));
          }
        }
      };

      apply_visitor(match{this, move(_current_task)}, result);
      RESUMABLE_STAT.running += high_resolution_clock::now() - r_start;
      ++RESUMABLE_STAT.running_count;
    }
  }
}
}
