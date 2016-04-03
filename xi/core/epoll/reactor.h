#pragma once

#include "xi/ext/configure.h"
#include "xi/core/epoll/event_loop.h"
#include "xi/core/reactor.h"

namespace xi {
namespace core {
  namespace epoll {

    class reactor : public xi::core::reactor {
    public:
      reactor(mut< core::shard >);
      void poll() override;
      own< xi::core::event > create_event(mut< event_handler >) override;

    private:
      own< event_loop > _event_loop;
    };
  }
}
}
