#pragma once

#include "xi/ext/configure.h"
#include "xi/core/sleep_queue.h"
#include "xi/core/worker_queue.h"

namespace xi {
namespace core {
  namespace v2 {

    class netpoller;
    class shared_queue;
    class scheduler;
    class execution_budget;

    class worker final {
    public:
      struct config {
        usize netpoll_max;
        usize scheduler_dequeue_max;
        usize loop_budget_allocation;
        usize max_loop_budget_allocation;
        usize isolation_budget_allocation;
        usize nr_spins_before_idle;
        struct {
          nanoseconds upper_bound_ready_queue;
          nanoseconds upper_bound_fast_queue;
        } timer_bounds;
      };
      static config DEFAULT_CONFIG;

    private:
      own< resumable > _current_task;
      mut< netpoller > _netpoller;
      mut< shared_queue > _scheduler_queue;
      mut< scheduler > _scheduler;
      u64 _index;
      config _config;

      worker_queue _port_queue;
      worker_queue _ready_queue;
      local_sleep_queue _local_sleep_queue;

    public:
      worker(mut< netpoller > n,
             mut< shared_queue > cq,
             mut< scheduler > s,
             u64 i,
             config c = DEFAULT_CONFIG);

    public:
      void run();

      void sleep_current(nanoseconds);
      void await_readable(i32 fd);
      void await_writable(i32 fd);
      u64 index() const;
      mut< worker_queue > port_queue();
      mut< worker_queue > ready_queue();

    private:
      void _block_resumable_on_sleep(own< resumable >, nanoseconds);
      void _run_task_from_queue(mut< worker_queue >, mut< execution_budget >);
    };

    inline void worker::sleep_current(nanoseconds ns) {
      assert(is_valid(_current_task));
      _current_task->yield(resumable::blocked::sleep{ns});
    }

    inline void worker::await_readable(i32 fd) {
      assert(is_valid(_current_task));
      _current_task->yield(
          resumable::blocked::port{fd, resumable::blocked::port::READ});
    }

    inline void worker::await_writable(i32 fd) {
      assert(is_valid(_current_task));
      _current_task->yield(
          resumable::blocked::port{fd, resumable::blocked::port::WRITE});
    }

    inline u64 worker::index() const {
      return _index;
    }

    inline mut< worker_queue > worker::port_queue() {
      return edit(_port_queue);
    }

    inline mut< worker_queue > worker::ready_queue() {
      return edit(_ready_queue);
    }

    // assigned by scheduler
    extern thread_local mut< worker > LOCAL_WORKER;
  }
}
}
