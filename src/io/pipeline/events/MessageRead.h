#pragma once

#include "ext/Configure.h"
#include "io/DataMessage.h"
#include "io/pipeline/events/Event.h"

namespace xi {
namespace io {
  namespace pipeline {

    class MessageRead : public UpstreamEvent {
    public:
      MessageRead(own< Message > msg) : _message(move(msg)) {}

      mut< Message > message() noexcept { return edit(_message); }
      own< Message > extractMessage() noexcept { return move(_message); }

    private:
      own< Message > _message;
    };
  }
}
} // namespace pipeline // namespace io // namespace xi
