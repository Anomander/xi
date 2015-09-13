#pragma once

#include "xi/io/pipeline/SimpleInboundPipelineHandler.h"
#include "xi/ext/Configure.h"

namespace xi {
namespace io {
  namespace pipeline {
    template < class Msg, class F >
    auto makeInboundHandler(F&& f) {
      struct Handler : public SimpleInboundPipelineHandler< Msg > {
        Handler(F&& f) : _f(forward< F >(f)) {}

        void messageReceived(mut< HandlerContext > cx, own< Msg > msg) override { _f(cx, move(msg)); }

      private:
        F _f;
      };

      return make< Handler >(forward< F >(f));
    }

    template < class E, class F >
    auto makeHandler(F&& f) {
      struct Handler : public PipelineHandler {
        Handler(F&& f) : _f(forward< F >(f)) {}

        void handleEvent(mut< HandlerContext > cx, own< E > msg) override { _f(cx, move(msg)); }

      private:
        F _f;
      };

      return make< Handler >(forward< F >(f));
    }
  }
}
}
