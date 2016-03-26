#include "xi/core/libevent/reactor.h"
#include "xi/core/libevent/event.h"

namespace xi {
namespace core {
  namespace libevent {

    reactor::reactor(mut< core::shard > s)
      : xi::core::reactor(s), _event_loop(make< event_loop >()) {
    }

    void reactor::poll() {
      _event_loop->dispatch_events(false);
    }

    own< xi::core::event > reactor::create_event(
        mut< event_handler > handler) {
      return make< event >(
          edit(_event_loop), handler->expected_state(), handler);
    }
  }
}
}
