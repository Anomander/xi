#pragma once

#include "xi/async/EventHandler.h"

struct event;

namespace xi {
namespace async {
  namespace libevent {

    class EventLoop;

    class Event : public async::Event {
    public:
      Event(mut< EventLoop >, EventState, mut< async::EventHandler >);
      void cancel() override;
      void arm() override;
      void changeState(EventState) override;
      void addState(EventState) override;
      void removeState(EventState) override;
      bool isActive() override;

    private:
      mut< EventLoop > _loop;
      mut< async::EventHandler > _handler;
      EventState _state;
      struct event* _event = nullptr;
    };
  }
}
}
