#include "xi/core/event_handler.h"
#include "xi/core/async_defer.h"
#include "xi/core/reactor.h"

namespace xi {
namespace core {

  void event_handler::attach_reactor(mut< reactor > reactor) {
    _reactor = some(reactor);
    _event   = reactor->create_event(this);
    _event->arm();
  }

  void event_handler::detach_reactor() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
  }

  void event_handler::cancel() {
    if (is_active()) {
      _event->cancel();
      defer(this, [this]() mutable {
        release(move(_event));
        if (_reactor.is_some()) {
          _reactor.unwrap()->detach_handler(this);
        }
      });
    }
  }

  bool event_handler::is_active() const noexcept {
    return _event && _event->is_active();
  }

  void io_handler::handle(event_state state) {
    if (state & kWrite) {
      handle_write();
    }

    if (!is_active()) {
      return;
    }

    if (state & kRead) {
      handle_read();
    }

    if (state & kClose) {
      handle_close();
    }

    if (state & kError) {
      handle_error();
    }
  }

  void io_handler::expect_read(bool flag) {
    if (flag) {
      _event->add_state(kRead);
    } else {
      _event->remove_state(kRead);
    }
  }

  void io_handler::expect_write(bool flag) {
    if (flag) {
      _event->add_state(kWrite);
    } else {
      _event->remove_state(kWrite);
    }
  }

  opt< milliseconds > io_handler::expected_timeout() const noexcept {
    return none;
  }
}
}
