#pragma once

#include "xi/ext/Configure.h"
#include "xi/async/Reactor.h"
#include "xi/async/libevent/EventLoop.h"

namespace xi {
namespace async {
  namespace libevent {

    class Reactor : public async::Reactor {
    public:
      Reactor();
      void poll() override;
      own< async::Event > createEvent(mut< EventHandler >) override;

    private:
      own< EventLoop > _eventLoop;
    };
  }
}
}
