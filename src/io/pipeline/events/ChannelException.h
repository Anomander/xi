#pragma once

#include "ext/Configure.h"
#include "io/pipeline/events/Event.h"

namespace xi {
namespace io {
  namespace pipeline {

    class ChannelException : public UpstreamEvent {
    public:
      ChannelException(exception_ptr ex) : _exception(ex) {
      }

      exception_ptr exception() noexcept {
        return _exception;
      }

    private:
      exception_ptr _exception;
    };
  }
}
}
