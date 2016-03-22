#pragma once

#include "xi/async/event_handler.h"

struct event_base;
struct event;

namespace xi {
namespace async {
  namespace libevent {

    class event_loop : virtual public ownership::std_shared {
      struct event_base *_event_base;

    public:
      event_loop();
      ~event_loop();

      void dispatch_events(bool blocking);

    public:
      struct ::event *prepare_event(struct ::event *e,
                                    event_state state,
                                    xi::async::event_handler *arg) noexcept;

    private:
      static void event_callback(int, short, void *) noexcept;
    };
  }
}
}
