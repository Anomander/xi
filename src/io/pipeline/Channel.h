#pragma once

#include "ext/Configure.h"
#include "io/DataMessage.h"
#include "io/pipeline/Pipeline.h"

namespace xi {
namespace io {
namespace pipeline {

class Channel : public io::Channel {
protected:
  virtual ~Channel() noexcept = default;

public:
  mut< Pipeline > pipeline() noexcept { return edit(_pipeline); }

public:
  void close() final override { pipeline()->channelClosed(); }
  void write(own< Message > msg) final override { pipeline()->fire(MessageWrite(move(msg))); }

protected:
  friend class HeadPipelineHandler;
  friend class TailPipelineHandler;

  virtual void doWrite(own< DataMessage >) = 0;
  virtual void doClose() = 0;

private:
  own< Pipeline > _pipeline = make< Pipeline >(this);
};
}
}
}
