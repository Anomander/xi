#pragma once

#include "ext/Configure.h"
#include "async/EventHandler.h"

namespace xi {
namespace async {
  struct Reactor : public virtual ownership::StdShared {
    virtual ~Reactor() = default;
    virtual void poll() = 0;
    virtual own< Event > createEvent(mut< EventHandler >) = 0;

    void attachHandler(own< EventHandler > handler) {
      handler->attachReactor(this);
      _handlers[addressOf(handler)] = move(handler);
    }

    void detachHandler(mut< EventHandler > handler) {
      std::cout << __PRETTY_FUNCTION__ << std::endl;
      handler->detachReactor();
      auto it = _handlers.find(handler);
      if (end(_handlers) != it) {
        auto handler = move(it->second);
        _handlers.erase(it);
        std::cout << handler.use_count() << std::endl;
      }
    }

  private:
    unordered_map< addressOf_t< own< EventHandler > >, own< EventHandler > > _handlers;
  };
}
}
