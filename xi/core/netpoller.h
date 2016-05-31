#pragma once

#include "xi/ext/configure.h"
#include "xi/core/worker_queue.h"

namespace xi {
namespace core {
  namespace v2 {
    class netpoller : public virtual ownership::unique {
      class impl;
      shared_ptr< impl > _impl;

    public:
      void start();
      void stop();
      void await_readable(i32, own< resumable >);
      void await_writable(i32, own< resumable >);
      usize poll_into(mut< worker_queue >, usize);
      usize blocking_poll_into(mut< worker_queue >, usize);
      void unblock_one(); // TODO: Feels weird here, should probably be
                           // somewhere else
    };
  }
}
}
