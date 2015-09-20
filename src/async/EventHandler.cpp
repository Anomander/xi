#include "xi/async/EventHandler.h"
#include "xi/async/Reactor.h"

namespace xi {
namespace async {

  void EventHandler::attachReactor(mut< Reactor > reactor) {
    _reactor = reactor;
    _event = reactor->createEvent(this);
    _event->arm();
  }

  void EventHandler::detachReactor() { std::cout << __PRETTY_FUNCTION__ << std::endl; }

  void EventHandler::cancel() {
    if (isActive()) {
      _event->cancel();
      defer([this]() mutable {
        release(move(_event));
        if (_reactor) {
          _reactor.get()->detachHandler(this);
        }
      });
    }
  }

  bool EventHandler::isActive() const noexcept { return _event && _event->isActive(); }

  void IoHandler::handle(EventState state) {
    if (state & kWrite) {
      handleWrite();
    }

    if (!isActive())
      return;

    if (state & kRead) {
      handleRead();
    }
  }

  void IoHandler::expectRead(bool flag) {
    if (flag) {
      _event->addState(kRead);
    } else {
      _event->addState(kRead);
    }
  }

  void IoHandler::expectWrite(bool flag) {
    if (flag) {
      _event->addState(kWrite);
    } else {
      _event->addState(kWrite);
    }
  }

  opt< milliseconds > IoHandler::expectedTimeout() const noexcept { return none; }
}
}
