#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/DataMessage.h"
#include "xi/io/pipeline/Pipeline.h"

namespace xi {
namespace io {
  namespace pipeline {

    class Channel : public io::Channel {
    protected:
      virtual ~Channel() noexcept = default;

    public:
      mut< Pipeline > pipeline() noexcept { return edit(_pipeline); }

    public:
      void close() final override { pipeline()->fire(ChannelClosed()); }
      void write(own< Message > msg) final override { pipeline()->fire(MessageWrite(move(msg))); }
      void write(ByteRange range) { doWrite(range); }

    protected:
      friend class HeadPipelineHandler;
      friend class TailPipelineHandler;

      virtual void doWrite(ByteRange) = 0;
      virtual void doClose() = 0;

    private:
      own< Pipeline > _pipeline = make< Pipeline >(this);
    };
  }
}
}
