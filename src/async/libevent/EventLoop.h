#pragma once

#include "async/libevent/EventHandler.h"

struct event_base;

namespace xi {
  namespace async {
    namespace libevent {
      class EventLoop
        : virtual public ownership::StdShared
      {
        struct event_base * _eventBase;
      public:
        EventLoop();
        ~EventLoop();

        void dispatchEvents(bool blocking);

      public:
        using Callback = void(int,short,void*);
        void prepareEvent(struct event**, int, short, EventHandler*) noexcept;
        void cancelEvent(struct event*) noexcept;
        void armEvent(struct event*, opt<milliseconds>) noexcept;

      private:
        static void eventCallback(int, short, void*) noexcept;
      };
    }
  }
}
