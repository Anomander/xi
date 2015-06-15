#pragma once

#include "ext/Configure.h"

#include <event2/event.h>

namespace xi {
  namespace async {
    namespace libevent {
      class EventLoop;
      class Executor;

      class EventHandler
        : virtual public ownership::StdShared
      {
      public:
        enum State : short {
          kNone = 0,
          kRead = EV_READ,
          kWrite = EV_WRITE,
          kReadWrite = kRead | kWrite,
          kTimeout = EV_TIMEOUT,
        };

      public:
        EventHandler(int descriptor)
          : _descriptor(descriptor)
        {}
        virtual ~EventHandler() {
          std::cout << "~EventHandler " << this << std::endl;
          if (_event) {
            event_free(_event);
          }
        }
        // virtual ~EventHandler() noexcept = default;
        virtual void cancel() noexcept;
        virtual void remove() noexcept;
        virtual void handle(State) = 0;
        virtual void attachExecutor(mut <Executor> loop);
        virtual void detachExecutor();

      protected:
        virtual bool isActive () const noexcept;
        virtual State expectedEvents() const noexcept = 0;
        virtual opt<milliseconds> expectedTimeout() const noexcept = 0;
        void reArm();

      private:
        struct event * _event = nullptr;
        int _descriptor = -1;
        bool _active = false;
        opt<mut<Executor>> _executor;
      };

      class IoHandler
        : public EventHandler {
      public:
        using EventHandler::EventHandler;

        void expectRead(bool) noexcept;
        void expectWrite(bool) noexcept;

      protected:
        virtual void handleRead() = 0;
        virtual void handleWrite() = 0;

        void handle(State) override;
        State expectedEvents() const noexcept override;
        opt<milliseconds> expectedTimeout() const noexcept override;

      private:
        State _expectedEvents = kRead;
      };
    }
  }
}
