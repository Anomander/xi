#pragma once

#include "ext/Configure.h"
#include "io/pipeline/events/Event.h"

namespace xi {
namespace io {
  namespace pipeline {

    class ChannelError : public UpstreamEvent {
    public:
      ChannelError(error_code error) : _error(error) {}

      error_code error() noexcept { return _error; }

    private:
      error_code _error;
    };
  }
}
}
