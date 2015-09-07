#include "async/EventHandler.h"
#include "async/Reactor.h"

namespace xi {
namespace async {

  void EventHandler::attachReactor(mut< Reactor > reactor) {
    _reactor = reactor;
    _event = reactor->createEvent(this);
    _event->arm();
    _active = true;
  }

  void EventHandler::detachReactor() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    cancel();
    _active = false;
  }

  void EventHandler::cancel() noexcept {
    if (_reactor) {
      _event->cancel();
      release(move(_event));
    }
    _active = false;
  }

  void EventHandler::remove() noexcept {
    if (_reactor) {
      _reactor.get()->detachHandler(this);
      _reactor = none;
    }
    _active = false;
  }

  bool EventHandler::isActive() const noexcept { return _active; }

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

  void IoHandler::expectRead(bool flag) noexcept {
    if (flag) {
      _event->addState(kRead);
    } else {
      _event->addState(kRead);
    }
  }

  void IoHandler::expectWrite(bool flag) noexcept {
    if (flag) {
      _event->addState(kWrite);
    } else {
      _event->addState(kWrite);
    }
  }

  opt< milliseconds > IoHandler::expectedTimeout() const noexcept { return none; }
}
}
