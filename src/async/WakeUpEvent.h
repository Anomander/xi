#pragma once

#include "async/EventHandler.h"
#include "ext/Configure.h"

namespace xi {
namespace async {

  class WakeUpEvent : public IoHandler {
  public:
    WakeUpEvent();

    void fire() noexcept;

  protected:
    void handleRead() override;
    void handleWrite() final override {
    }
  };
}
}
