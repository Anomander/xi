#pragma once

#include "xi/ext/configure.h"
#include "xi/core/message_bus.h"

namespace xi {
namespace core {

  struct message_bus::message : public virtual ownership::unique {
  public:
    XI_CLASS_DEFAULTS(message, no_copy, no_move, ctor, virtual_dtor);
    virtual void process_request() = 0;
    virtual void process_reply()   = 0;
  };

  template < class F >
  struct remote_message : message_bus::message {
    F _delegate;
    promise< future_result< F > > _promise;
    struct {
      opt< future_result< F > > result;
      exception_ptr exception;
    } _remote;

  public:
    remote_message(F&& f) : _delegate(move(f)) {
    }

    void process_request() override {
      try {
        _remote.result = some(callable_applier<>::apply(move(_delegate)));
      } catch (...) {
        _remote.exception = current_exception();
      }
    }

    void process_reply() override {
      _remote.result.map_or([&] { _promise.set(move(_remote.exception)); },
                            [&](auto&& val) { _promise.set(move(val)); });
    }

    auto get_future() {
      return _promise.get_future();
    }
  };

  template < class F >
  future< future_result< F > > message_bus::submit(F&& f) {
    auto msg = make< remote_message< decay_t< F > > >(move(f));
    auto fut = msg->get_future();
    _submit(move(msg));
    return fut;
  }
}
}
