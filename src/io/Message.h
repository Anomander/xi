#pragma once

#include "ext/Configure.h"

namespace xi {
  namespace io {

    class ClientChannelConnected;
    class DataMessage;

    struct Message
      : FastCastableGroup
      <
        ClientChannelConnected
      , DataMessage
      >
      , public ownership::Unique
    {
      Message () = default;
      virtual ~Message() noexcept = default;
    };

  }
}
