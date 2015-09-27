#pragma once

#include "xi/ext/configure.h"
#include "xi/async/event_handler.h"
#include "xi/async/async.h"

namespace xi {
namespace async {

  struct reactor : public async< reactor >,
                   public virtual ownership::std_shared {
    virtual ~reactor() = default;
    virtual void poll() = 0;
    virtual own< event > create_event(mut< event_handler >) = 0;

    virtual void handler_cancelled(mut< event_handler > handler) {
      _handlers.erase(handler);
    }

    void attach_handler(own< event_handler > handler) {
      auto mut_handler = edit(handler);
      _handlers[address_of(handler)] = move(handler);
      XI_SCOPE(failure) { _handlers.erase(address_of(mut_handler)); };
      mut_handler->attach_executor(share_executor());
      mut_handler->attach_reactor(this);
    }

    void detach_handler(mut< event_handler > handler) {
      std::cout << __PRETTY_FUNCTION__ << std::endl;
      auto it = _handlers.find(handler);
      if (end(_handlers) != it) {
        handler->detach_reactor();
        _handlers.erase(it);
      }
    }

  private:
    unordered_map< address_of_t< own< event_handler > >, own< event_handler > >
        _handlers;
  };
}
}
