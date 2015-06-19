#pragma once

#include "async/Executor.h"
#include "async/WakeUpEvent.h"
#include "async/libevent/Event.h"
#include "async/libevent/EventLoop.h"
#include "ext/Configure.h"

namespace xi {
  namespace async {
    namespace libevent {

      class Executor
        : public EventLoop
        , public async::Executor
      {
      public:
        Executor() {
          _wakeUp = make <WakeUpEvent> ();
          attachHandler (_wakeUp);
        }

        own<async::Event> createEvent(mut<async::EventHandler> handler) override {
          return make <Event> (this, handler->expectedState(), handler);
        }

        void run() {
          while(true) {
            dispatchEvents(true);
          }
        }

        void runOnce() {
          dispatchEvents(false);
        }

      private:
        unordered_map <addressOf_t <own<EventHandler>>, own<EventHandler>> _handlers;
        own <WakeUpEvent> _wakeUp;
      };

    }
  }
}
