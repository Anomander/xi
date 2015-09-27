#include "xi/async/libevent/reactor.h"
#include "xi/async/libevent/event.h"

namespace xi {
namespace async {
  namespace libevent {

    reactor::reactor() : _event_loop(make< event_loop >()) {}

    void reactor::poll() { _event_loop->dispatch_events(false); }

    own< xi::async::event > reactor::create_event(
        mut< event_handler > handler) {
      return make< event >(edit(_event_loop), handler->expected_state(),
                           handler);
    }
  }
}
}
