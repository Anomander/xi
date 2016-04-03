#pragma once

#include "xi/core/event_handler.h"

struct event_base;
struct event;

namespace xi {
namespace core {
  namespace epoll {

    class event_loop : virtual public ownership::std_shared {
      i32 _epoll_descriptor = -1;

    public:
      event_loop();
      ~event_loop();

      void dispatch_events(bool blocking);

    public:
      void arm_event(i32 desc, event_state state, mut<xi::core::event_handler> h);
      void mod_event(i32 desc, event_state state, mut<xi::core::event_handler> h);
      void del_event(i32 desc);

    private:
      static void event_callback(i32, i32, void *) noexcept;
    };
  }
}
}
