#include "async/libevent/Event.h"
#include "async/libevent/EventLoop.h"
#include "async/libevent/detail/EventStateToShort.h"

#include <event2/event.h>

namespace xi {
namespace async {
  namespace libevent {

    Event::Event(mut< EventLoop > loop, EventState state, mut< async::EventHandler > handler)
        : _loop(loop), _handler(handler), _state(state) {
      std::cout << "Event()" << std::endl;
      _event = _loop->prepareEvent(_event, _state, _handler);
      if (nullptr == _event) {
        throw std::runtime_error("Failed to create event.");
      }
    }

    void Event::cancel() {
      event_del(_event);
    }

    void Event::arm() {
      std::cout << "Event::arm" << std::endl;
      auto timeout = _handler->expectedTimeout();
      if (timeout == none) {
        event_add(_event, NULL);
      } else {
        auto&& ms = timeout.get();
        struct timeval tv = {ms.count() / 1000, ms.count() % 1000};
        event_add(_event, &tv);
      }
    }

    void Event::changeState(EventState state) {
      cancel();
      _state = state;
      _loop->prepareEvent(_event, _state, _handler);
      arm();
    }

    void Event::addState(EventState newState) {
      changeState(static_cast< EventState >(_state | newState));
    }

    void Event::removeState(EventState newState) {
      changeState(static_cast< EventState >(_state & ~newState));
    }
  }
}
}
