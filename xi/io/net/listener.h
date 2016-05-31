#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pollable_fd.h"
#include "xi/util/result.h"
#include "xi/core/channel.h"

namespace xi {
namespace io {
  namespace net {
    struct listener : public port {
      listener(u16 port);

      result< i32 > accept();

      core::channel< i32 > as_channel();
    };
  }
}
}
