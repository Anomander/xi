#include "async/libevent/EventHandler.h"
#include "async/libevent/Executor.h"

namespace xi {
  namespace async {
    namespace libevent {
      void EventHandler::attachExecutor (mut <Executor> executor) {
        std::cout << "EventHandler::attachExecutor" << std::endl;
        cancel();
        _executor = executor;
        reArm();
        _active = true;
      }

      void EventHandler::detachExecutor () {
        std::cout << "EventHandler::detachExecutor" << std::endl;
        cancel();
        _executor = none;
        _active = false;
      }

      void EventHandler::cancel() noexcept {
        std::cout << "EventHandler::cancel" << std::endl;
        if (_executor!= none) {
          _executor.get()->cancelEvent(_event);
        }
        _active = false;
      }

      void EventHandler::remove() noexcept {
        std::cout << "EventHandler::remove" << std::endl;
        if (_executor!= none) {
          _executor.get()->cancelEvent(_event);
          _executor.get()->detachHandler(this);
        }
        _active = false;
      }

      void EventHandler::reArm() {
        std::cout << "EventHandler::reArm" << std::endl;
        if (! _executor) return;
        auto executor = _executor.get();
        auto flag = static_cast<short> (expectedEvents());
        flag |= EV_PERSIST | EV_ET;
        executor->prepareEvent(& _event, _descriptor, flag, this);
        executor->armEvent(_event, expectedTimeout());
      }

      bool EventHandler::isActive () const noexcept {
        return _active;
      }

      void IoHandler::handle(State state) {
        if (state & kWrite) {
          handleWrite();
        }

        if (! isActive()) return;

        if (state & kRead) {
          handleRead();
        }
      }

      void IoHandler::expectRead(bool flag) noexcept {
        _expectedEvents = static_cast <State> (flag ? _expectedEvents | kRead : _expectedEvents & ~kRead);
        cancel();
        reArm();
      }

      void IoHandler::expectWrite(bool flag) noexcept {
        _expectedEvents = static_cast <State> (flag ? _expectedEvents | kWrite : _expectedEvents & ~kWrite);
        cancel();
        reArm();
      }

      auto IoHandler::expectedEvents() const noexcept -> State {
        return _expectedEvents;
      }

      opt<milliseconds> IoHandler::expectedTimeout() const noexcept {
        return none;
      }
    }
  }
}











