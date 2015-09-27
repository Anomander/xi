#pragma once

#include "xi/ext/configure.h"
#include "xi/io/data_message.h"
#include "xi/io/pipeline/pipeline.h"

namespace xi {
namespace io {
  namespace pipeline {

    class channel : public io::channel {
    protected:
      virtual ~channel() noexcept = default;

    public:
      mut< pipeline > get_pipeline() noexcept { return edit(_pipeline); }

    public:
      void close() final override { _pipeline->fire(channel_closed()); }
      void write(own< message > msg) final override {
        _pipeline->fire(message_write(move(msg)));
      }
      void write(byte_range range) { do_write(range); }

    protected:
      friend class head_pipeline_handler;
      friend class tail_pipeline_handler;

      virtual void do_write(byte_range) = 0;
      virtual void do_close() = 0;

    private:
      own< pipeline > _pipeline = make< pipeline >(this);
    };
  }
}
}
