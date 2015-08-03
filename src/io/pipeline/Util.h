#pragma once

#include "io/pipeline/SimpleInboundPipelineHandler.h"
#include "ext/Configure.h"

namespace xi {
namespace io {
  namespace pipeline {
    template < class Msg, class F >
    auto makeInboundHandler(F&& f) {
      struct InboundHandler : public SimpleInboundPipelineHandler< Msg > {
        InboundHandler(F&& f) : _f(forward< F >(f)) {}

        void messageReceived(mut< HandlerContext > cx, own< Msg > msg) override { _f(cx, move(msg)); }

      private:
        F _f;
      };

      return make< InboundHandler >(forward< F >(f));
    }
  }
}
}
