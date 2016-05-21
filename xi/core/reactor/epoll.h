#pragma once

#include "xi/ext/configure.h"
#include "xi/core/detail/intrusive.h"
#include "xi/core/reactor/abstract_reactor.h"

namespace xi {
namespace core {
  class resumable;
  class abstract_worker;

  namespace reactor {
    class epoll : public abstract_reactor {
      i32 _epoll     = -1;
      i32 _wakeup_fd = -1;
      i32 _timer_fd  = -1;
      // detail::block_queue_type* _signal_queues;
      pthread_t _thread_id;
      timer_t _quota_timer;

    public:
      epoll();
      ~epoll();

      void poll_for(nanoseconds) override;
      void attach_pollable(resumable*, i32) override;
      void detach_pollable(resumable*, i32) override;
      void await_signal(resumable*, i32) override;
      void await_readable(resumable*, i32) override;
      void await_writable(resumable*, i32) override;
      void maybe_wakeup();
      void begin_task_quota_monitor(nanoseconds);
      void end_task_quota_monitor();
    };
  }
}
}
