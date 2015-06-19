#pragma once

#include "async/Event.h"
#include "ext/Configure.h"

namespace xi {
  namespace async {
    class Executor;

    class EventHandler
      : virtual public ownership::StdShared
    {
    public:
    public:
      virtual ~EventHandler() noexcept = default;
      virtual void cancel() noexcept;
      virtual void remove() noexcept;
      virtual void attachExecutor(mut <Executor> loop);
      virtual void detachExecutor();
      virtual bool isActive () const noexcept;

      virtual void handle(EventState) = 0;
      virtual opt<milliseconds> expectedTimeout() const noexcept = 0;
      virtual EventState expectedState() const noexcept = 0;
      virtual int descriptor() const noexcept = 0;

    protected:
      own <Event> _event;
      opt <mut <Executor>> _executor;
      bool _active = false;
    };

    class IoHandler
      : public EventHandler
    {
    public:
      IoHandler (int descriptor)
        : _descriptor (descriptor)
      {}

      void expectRead(bool) noexcept;
      void expectWrite(bool) noexcept;

    protected:
      virtual void handleRead() = 0;
      virtual void handleWrite() = 0;

      void handle(EventState) override;
      opt<milliseconds> expectedTimeout() const noexcept override;
      int descriptor() const noexcept override {return _descriptor;}
      EventState expectedState() const noexcept override {return kRead;}

    private:
      int _descriptor = -1;
    };
  }
}
