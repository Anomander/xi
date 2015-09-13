#pragma once

#include "xi/async/EventHandler.h"

struct event_base;
struct event;

namespace xi {
namespace async {
  namespace libevent {

    class EventLoop : virtual public ownership::StdShared {
      struct event_base* _eventBase;

    public:
      EventLoop();
      ~EventLoop();

      void dispatchEvents(bool blocking);

    public:
      struct event* prepareEvent(struct event* e, EventState state, async::EventHandler* arg) noexcept;

    private:
      static void eventCallback(int, short, void*) noexcept;
    };
  }
}
}
