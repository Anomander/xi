#pragma once

#include "ext/Configure.h"
#include "io/DataMessage.h"
#include "io/pipeline/events/Event.h"

namespace xi {
namespace io {
namespace pipeline {

class MessageWrite : public DownstreamEvent {
public:
  MessageWrite(own< Message > msg) : _message(move(msg)) {}

  mut< Message > message() noexcept { return edit(_message); }
  own< Message > extractMessage() noexcept { return move(_message); }

private:
  own< Message > _message;
};
}
}
}
