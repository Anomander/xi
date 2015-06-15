#pragma once

#include "io/SocketOption.h"

#include <netinet/in.h>

namespace xi {
  namespace io {

    using ReuseAddress = BooleanSocketOption <SOL_SOCKET, SO_REUSEADDR>;

  }
}
