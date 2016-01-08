#pragma once

#include "xi/ext/configure.h"
#include "xi/async/reactor.h"
#include "xi/async/libevent/event_loop.h"

namespace xi {
namespace async {
  namespace libevent {

    class reactor : public xi::async::reactor {
    public:
      reactor();
      void poll() override;
      own< xi::async::event > create_event(mut< event_handler >) override;

    private:
      own< event_loop > _event_loop;
    };
  }
}
}