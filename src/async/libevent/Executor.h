#pragma once

#include "async/libevent/EventLoop.h"
#include "io/AsyncChannel.h"
#include "ext/Configure.h"

namespace xi {
  namespace async {
    namespace libevent {

      class Executor
        : public EventLoop
        , virtual public ownership::StdShared
      {
      public:
        virtual ~Executor() noexcept {
        }

        void attachHandler(own<EventHandler> handler) {
          handler->attachExecutor (this);
          _handlers [addressOf (handler)] = move (handler);
        }

        void detachHandler(mut<EventHandler> handler) {
          handler->detachExecutor();
          auto it = _handlers.find (handler);
          if (end(_handlers) != it) {
            auto handler = move(it->second);
            _handlers.erase(it);
          }
        }

        void run() {
          while(_shouldRun.load(std::memory_order_acquire)) {
            dispatchEvents(false);
          }
        }

        void runOnce() {
          dispatchEvents(false);
        }

      private:
        atomic<bool> _shouldRun { true };
        unordered_map <addressOf_t <own<EventHandler>>, own<EventHandler>> _handlers;
      };

    }
  }
}
