#pragma once

#include "xi/ext/Configure.h"
#include "xi/async/Event.h"
#include "xi/async/Async.h"

namespace xi {
namespace async {
  class Reactor;

  class EventHandler : public Async< EventHandler >, virtual public ownership::StdShared {
  public:
    virtual ~EventHandler() noexcept = default;
    virtual void cancel();
    virtual void attachReactor(mut< Reactor >);
    virtual void detachReactor();
    virtual bool isActive() const noexcept;

    virtual void handle(EventState) = 0;
    virtual opt< milliseconds > expectedTimeout() const noexcept = 0;
    virtual EventState expectedState() const noexcept = 0;
    virtual int descriptor() const noexcept = 0;

  protected:
    own< Event > _event;
    opt< mut< Reactor > > _reactor;
  };

  class IoHandler : public EventHandler {
  public:
    IoHandler(int descriptor) : _descriptor(descriptor) {}

    void expectRead(bool);
    void expectWrite(bool);

  protected:
    virtual void handleRead() = 0;
    virtual void handleWrite() = 0;

    void handle(EventState) override;
    opt< milliseconds > expectedTimeout() const noexcept override;
    int descriptor() const noexcept override { return _descriptor; }
    EventState expectedState() const noexcept override { return kRead; }

  private:
    int _descriptor = -1;
  };
}
}
