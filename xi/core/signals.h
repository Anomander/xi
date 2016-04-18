#pragma once

#include "xi/core/shard.h"

#include <signal.h>

namespace xi {
namespace core {

  class shard::signals : public poller {
    unordered_map< i32, function< void() > > _handlers;
    atomic< u64 > _pending_signals {0};

  public:
    signals();
    ~signals();

    void handle(i32 signo, function< void() > handler);
    u32 poll() noexcept override;
  };
}
}
