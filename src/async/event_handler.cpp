#include "xi/async/event_handler.h"
#include "xi/async/reactor.h"

namespace xi {
namespace async {

  void event_handler::attach_reactor(mut< reactor > reactor) {
    _reactor = reactor;
    _event = reactor->create_event(this);
    _event->arm();
  }

  void event_handler::detach_reactor() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
  }

  void event_handler::cancel() {
    if (is_active()) {
      _event->cancel();
      defer([this]() mutable {
        release(move(_event));
        if (_reactor) { _reactor.get()->detach_handler(this); }
      });
    }
  }

  bool event_handler::is_active() const noexcept {
    return _event && _event->is_active();
  }

  void io_handler::handle(event_state state) {
    if (state & kWrite) { handle_write(); }

    if (!is_active()) return;

    if (state & kRead) { handle_read(); }
  }

  void io_handler::expect_read(bool flag) {
    if (flag) { _event->add_state(kRead); } else {
      _event->add_state(kRead);
    }
  }

  void io_handler::expect_write(bool flag) {
    if (flag) { _event->add_state(kWrite); } else {
      _event->add_state(kWrite);
    }
  }

  opt< milliseconds > io_handler::expected_timeout() const noexcept {
    return none;
  }
}
}
