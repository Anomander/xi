#pragma once

#include "async/EventHandler.h"
#include "ext/Configure.h"

namespace xi {
namespace async {

class Executor : virtual public ownership::StdShared {
public:
  virtual ~Executor() noexcept = default;

  void attachHandler(own< EventHandler > handler) {
    handler->attachExecutor(this);
    _handlers[addressOf(handler)] = move(handler);
  }

  void detachHandler(mut< EventHandler > handler) {
    handler->detachExecutor();
    auto it = _handlers.find(handler);
    if (end(_handlers) != it) {
      auto handler = move(it->second);
      _handlers.erase(it);
    }
  }

  virtual own< Event > createEvent(mut< EventHandler >) = 0;
  virtual void run() = 0;
  virtual void runOnce() = 0;

private:
  unordered_map< addressOf_t< own< EventHandler > >, own< EventHandler > > _handlers;
};
}
}
