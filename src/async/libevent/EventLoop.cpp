#include "async/libevent/EventLoop.h"
#include "async/libevent/detail/EventStateToShort.h"

#include <event2/event.h>

namespace xi {
namespace async {
namespace libevent {

EventLoop::EventLoop() : _eventBase(event_base_new()) {
  if (!_eventBase) {
    throw std::runtime_error("Unable to create event base.");
  }
}

EventLoop::~EventLoop() { event_base_free(_eventBase); }

void EventLoop::dispatchEvents(bool blocking) {
  event_base_loop(_eventBase, EVLOOP_ONCE | (blocking ? 0 : EVLOOP_NONBLOCK));
}

void EventLoop::eventCallback(int descriptor, short what, void* arg) noexcept {
  if (arg) {
    auto* handler = reinterpret_cast< EventHandler* >(arg);
    handler->handle(detail::ShortToEventState(what));
  }
}

struct event* EventLoop::prepareEvent(struct event* e, EventState state, async::EventHandler* arg) noexcept {
  auto s = detail::EventStateToShort(state);
  if (e == nullptr) {
    return event_new(_eventBase, arg->descriptor(), s, &EventLoop::eventCallback, arg);
  } else {
    event_assign(e, _eventBase, arg->descriptor(), s, &EventLoop::eventCallback, arg);
    return e;
  }
}
}
}
}
