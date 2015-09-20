#pragma once

#include "xi/ext/Configure.h"
#include "xi/async/EventHandler.h"
#include "xi/async/Async.h"

namespace xi {
namespace async {

  struct Reactor : public Async< Reactor >, public virtual ownership::StdShared {
    virtual ~Reactor() = default;
    virtual void poll() = 0;
    virtual own< Event > createEvent(mut< EventHandler >) = 0;

    virtual void handlerCancelled(mut< EventHandler > handler) { _handlers.erase(handler); }

    void attachHandler(own< EventHandler > handler) {
      auto mutHandler = edit(handler);
      _handlers[addressOf(handler)] = move(handler);
      XI_SCOPE(failure) { _handlers.erase(addressOf(mutHandler)); };
      mutHandler->attachExecutor(shareExecutor());
      mutHandler->attachReactor(this);
    }

    void detachHandler(mut< EventHandler > handler) {
      std::cout << __PRETTY_FUNCTION__ << std::endl;
      auto it = _handlers.find(handler);
      if (end(_handlers) != it) {
        handler->detachReactor();
        _handlers.erase(it);
      }
    }

  private:
    unordered_map< addressOf_t< own< EventHandler > >, own< EventHandler > > _handlers;
  };
}
}
