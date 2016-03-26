#include "xi/async/libevent/event.h"
#include "xi/async/libevent/detail/event_state_to_short.h"
#include "xi/async/libevent/event_loop.h"

#include <event2/event.h>

namespace xi {
namespace async {
  namespace libevent {

    event::event(mut< event_loop > loop,
                 event_state state,
                 mut< xi::async::event_handler > handler)
        : _loop(loop), _handler(handler), _state(state) {
      _event = _loop->prepare_event(_event, _state, _handler);
      if (nullptr == _event) {
        throw std::runtime_error("Failed to create event.");
      }
    }

    void event::cancel() {
      event_del((::event *)_event);
      _event = nullptr;
    }

    bool event::is_active() {
      return (nullptr != _event);
    }

    void event::arm() {
      auto timeout = _handler->expected_timeout();
      if (timeout == none) {
        event_add((::event *)_event, NULL);
      } else {
        auto &&ms         = timeout.unwrap();
        struct timeval tv = {ms.count() / 1000, ms.count() % 1000};
        event_add((::event *)_event, &tv);
      }
    }

    void event::change_state(event_state state) {
      cancel();
      _state = state;
      _event = _loop->prepare_event(_event, _state, _handler);
      arm();
    }

    void event::add_state(event_state new_state) {
      change_state(static_cast< event_state >(_state | new_state));
    }

    void event::remove_state(event_state new_state) {
      change_state(static_cast< event_state >(_state & ~new_state));
    }
  }
}
}
