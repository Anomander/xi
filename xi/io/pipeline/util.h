#pragma once

#include "xi/io/pipeline/simple_inbound_pipeline_handler.h"
#include "xi/ext/configure.h"

namespace xi {
namespace io {
  namespace pipeline {
    template < class M, class F > auto make_inbound_handler(F &&f) {
      struct handler : public simple_inbound_pipeline_handler< M > {
        handler(F &&f) : _f(forward< F >(f)) {}

        void message_received(mut< handler_context > cx,
                              own< M > msg) override {
          _f(cx, move(msg));
        }

      private:
        F _f;
      };

      return make< handler >(forward< F >(f));
    }

    template < class E, class F > auto make_handler(F &&f) {
      struct handler : public pipeline_handler {
        handler(F &&f) : _f(forward< F >(f)) {}

        void handle_event(mut< handler_context > cx, own< E > msg) override {
          _f(cx, move(msg));
        }

      private:
        F _f;
      };

      return make< handler >(forward< F >(f));
    }
  }
}
}
