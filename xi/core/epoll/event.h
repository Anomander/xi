#pragma once

#include "xi/core/event_handler.h"

struct event;

namespace xi {
  namespace core {
    namespace epoll {

      class event_loop;

      class event : public xi::core::event {
      public:
        event(mut< event_loop >, event_state, mut< xi::core::event_handler >);
        void cancel() override;
        void arm() override;
        void change_state(event_state) override;
        void add_state(event_state) override;
        void remove_state(event_state) override;
        bool is_active() override;

      private:
        mut< event_loop > _loop;
        mut< xi::core::event_handler > _handler;
        event_state _state;
        bool _is_active = false;
      };
    }
  }
}
