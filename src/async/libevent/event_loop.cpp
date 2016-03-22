#include "xi/async/libevent/event_loop.h"
#include "xi/async/libevent/detail/event_state_to_short.h"

#include <event2/event.h>

namespace xi {
namespace async {
  namespace libevent {

    event_loop::event_loop() : _event_base(event_base_new()) {
      if (!_event_base) {
        throw std::runtime_error("Unable to create event base.");
      }
    }

    event_loop::~event_loop() {
      event_base_free(_event_base);
    }

    void event_loop::dispatch_events(bool blocking) {
      event_base_loop(_event_base,
                      EVLOOP_ONCE | (blocking ? 0 : EVLOOP_NONBLOCK));
    }

    void event_loop::event_callback(int descriptor,
                                    short what,
                                    void *arg) noexcept {
      if (arg) {
        auto *handler = reinterpret_cast< event_handler * >(arg);
        handler->defer([ h = share(handler), what ] {
          h->handle(detail::short_to_event_state(what));
        });
      }
    }

    struct ::event *event_loop::prepare_event(
        struct ::event *e,
        event_state state,
        xi::async::event_handler *arg) noexcept {
      auto s = detail::event_state_to_short(state);
      if (e == nullptr) {
        return event_new(_event_base,
                         arg->descriptor(),
                         s,
                         &event_loop::event_callback,
                         arg);
      } else {
        event_assign(e,
                     _event_base,
                     arg->descriptor(),
                     s,
                     &event_loop::event_callback,
                     arg);
        return e;
      }
    }
  }
}
}
