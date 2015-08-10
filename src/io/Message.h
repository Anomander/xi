#pragma once

#include "ext/Configure.h"

namespace xi {
namespace io {

  class ClientChannelConnected;
  class DataMessage;
  class DataAvailable;

  struct Message : FastCastableGroup< ClientChannelConnected, DataMessage, DataAvailable >, public ownership::Unique {
    Message() = default;
    virtual ~Message() noexcept = default;
  };
}
}
