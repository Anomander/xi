#include "async/libevent/EventLoop.h"

#include <event2/event.h>

namespace xi {
  namespace async {
    namespace libevent {
      EventLoop::EventLoop()
        : _eventBase(event_base_new())
      {
        if (! _eventBase) {
          throw std::runtime_error("Unable to create event base.");
        }
      }

      EventLoop::~EventLoop() {
        event_base_free (_eventBase);
      }

      void EventLoop::dispatchEvents(bool blocking) {
        event_base_loop(_eventBase, EVLOOP_ONCE | (blocking ? 0 : EVLOOP_NONBLOCK));
      }

      void EventLoop::eventCallback(int descriptor, short what, void* arg) noexcept {
        try {
          auto * handler = reinterpret_cast<EventHandler*>(arg);
          handler->handle(static_cast<EventHandler::State>(what));
        } catch (...) {
          //TODO: Add exception handling.
        }
      }

      void EventLoop::prepareEvent(struct event** e, int d, short f, EventHandler* arg) noexcept {
        if (* e == nullptr) {
          * e = event_new(_eventBase, d, f, &EventLoop::eventCallback, arg);
        } else {
          event_assign(* e, _eventBase, d, f, &EventLoop::eventCallback, arg);
        }
      }

      void EventLoop::cancelEvent(struct event* e) noexcept {
        event_del(e);
      }

      void EventLoop::armEvent(struct event* e, opt<milliseconds> timeout) noexcept {
        if (timeout == none) {
          event_add(e, NULL);
        } else {
          auto && ms = timeout.get();
          struct timeval tv = { ms.count() / 1000, ms.count() % 1000 };
          event_add(e, & tv);
        }
      }
    }
  }
}
