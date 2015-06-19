#include "async/EventHandler.h"
#include "async/Executor.h"

namespace xi {
  namespace async {

      void EventHandler::attachExecutor (mut <Executor> executor) {
        _executor = executor;
        _event = executor->createEvent(this);
        _event->arm();
        _active = true;
      }

      void EventHandler::detachExecutor () {
        cancel();
        _active = false;
      }

      void EventHandler::cancel() noexcept {
        _event->cancel();
        _active = false;
      }

      void EventHandler::remove() noexcept {
        if (_executor) {
          _executor.get()->detachHandler(this);
        }
        release (move(_event));
        _active = false;
      }

      bool EventHandler::isActive () const noexcept {
        return _active;
      }

      void IoHandler::handle(EventState state) {
        if (state & kWrite) {
          handleWrite();
        }

        if (! isActive()) return;

        if (state & kRead) {
          handleRead();
        }
      }

      void IoHandler::expectRead(bool flag) noexcept {
        if (flag) {
          _event->addState (kRead);
        } else {
          _event->addState (kRead);
        }
      }

      void IoHandler::expectWrite(bool flag) noexcept {
        if (flag) {
          _event->addState (kWrite);
        } else {
          _event->addState (kWrite);
        }
      }

      opt<milliseconds> IoHandler::expectedTimeout() const noexcept {
        return none;
      }
    }
}
