#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/coroutine.h"
#include "xi/core/async.h"
#include "xi/core/event.h"

namespace xi {
namespace core {
  class reactor;

  class event_handler : public async< event_handler >,
                        virtual public ownership::std_shared {
  public:
    virtual ~event_handler() noexcept = default;
    virtual void cancel();
    virtual void attach_reactor(mut< reactor >);
    virtual void detach_reactor();
    virtual bool is_active() const noexcept;

    virtual void handle(u16 state)                                = 0;
    virtual opt< milliseconds > expected_timeout() const noexcept = 0;
    virtual event_state expected_state() const noexcept           = 0;
    virtual int descriptor() const noexcept                       = 0;

  protected:
    own< event > _event;
    opt< mut< reactor > > _reactor;
  };

  class io_handler : public event_handler {
  public:
    void expect_read(bool);
    void expect_write(bool);

  protected:
    virtual void handle_read()  = 0;
    virtual void handle_write() = 0;
    virtual void handle_close() = 0;
    virtual void handle_error() = 0;

    void handle(u16 state) override;
    opt< milliseconds > expected_timeout() const noexcept override;
    int descriptor() const noexcept override {
      return _descriptor;
    }
    void descriptor(int d) noexcept {
      _descriptor = d;
    }
    event_state expected_state() const noexcept override {
      return kRead;
    }

  private:
    int _descriptor = -1;
  };

  class coro_io_handler : public io_handler {
    using coro_t = symmetric_coroutine< void >::call_type;

    coro_t _routine;

  public:
    coro_io_handler(coro_t&& c) : _routine(move(c)) {
    }
    void handle_read() override {
      _routine();
    }
    void handle_write() override {
      _routine();
    }
    void handle_close() override {
      _routine();
    }
    void handle_error() override {
      _routine();
    }
  };
}
}
