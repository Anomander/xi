#include "xi/async/libevent/Reactor.h"
#include "xi/async/libevent/Event.h"

namespace xi {
namespace async {
  namespace libevent {

    Reactor::Reactor() : _eventLoop(make< EventLoop >()) {}

    void Reactor::poll() { _eventLoop->dispatchEvents(false); }

    own< async::Event > Reactor::createEvent(mut< EventHandler > handler) {
      return make< Event >(edit(_eventLoop), handler->expectedState(), handler);
    }
  }
}
}
