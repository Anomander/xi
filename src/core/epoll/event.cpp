#include "xi/core/epoll/event.h"
#include "xi/core/epoll/detail/event_state_to_short.h"
#include "xi/core/epoll/event_loop.h"

#include <sys/epoll.h>

namespace xi {
namespace core {
  namespace epoll {

    event::event(mut< event_loop > loop,
                 event_state state,
                 mut< xi::core::event_handler > handler)
        : _loop(loop), _handler(handler), _state(state) {
    }

    void event::cancel() {
      _loop->del_event(_handler->descriptor());
      _is_active = false;
    }

    bool event::is_active() {
      return _is_active;
    }

    void event::arm() {
      _loop->arm_event(_handler->descriptor(), _state, _handler);
      _is_active = true;
    }

    void event::change_state(event_state state) {
      _state = state;
      _loop->mod_event(_handler->descriptor(), _state, _handler);
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
