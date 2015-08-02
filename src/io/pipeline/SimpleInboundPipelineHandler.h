#pragma once

#include "ext/Configure.h"
#include "io/DataMessage.h"
#include "io/pipeline/PipelineHandler.h"
#include "io/pipeline/HandlerContext.h"

namespace xi {
namespace io {
  namespace pipeline {

    template < class T >
    class SimpleInboundPipelineHandler : public PipelineHandler {
    public:
      // TODO: Fix cast
      void handleEvent(mut< HandlerContext > cx, own< MessageRead > data) override {
        auto message = fast_cast< T >(data.message());
        if (message) {
          data.extractMessage().release();
          messageReceived(cx, own< T >{message});
        } else {
          cx->forward(move(data));
        }
      }

      virtual void messageReceived(mut< HandlerContext > cx, own< T > msg) = 0;
    };
  }
}
}
