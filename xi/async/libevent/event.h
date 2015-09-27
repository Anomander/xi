#pragma once

#include "xi/async/event_handler.h"

struct event;

namespace xi {
namespace async {
  namespace libevent {

    class event_loop;

    class event : public xi::async::event {
    public:
      event(mut< event_loop >, event_state, mut< xi::async::event_handler >);
      void cancel() override;
      void arm() override;
      void change_state(event_state) override;
      void add_state(event_state) override;
      void remove_state(event_state) override;
      bool is_active() override;

    private:
      mut< event_loop > _loop;
      mut< xi::async::event_handler > _handler;
      event_state _state;
      ::event *_event = nullptr;
    };
  }
}
}
