#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {

  class client_channel_connected;
  class data_message;

  struct message
      : fast_castable_group< client_channel_connected, data_message >,
        public ownership::unique {
    message() = default;
    virtual ~message() noexcept = default;
  };
}
}
