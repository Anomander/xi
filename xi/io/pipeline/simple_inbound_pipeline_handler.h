#pragma once

#include "xi/ext/configure.h"
#include "xi/io/data_message.h"
#include "xi/io/pipeline/pipeline_handler.h"
#include "xi/io/pipeline/handler_context.h"

namespace xi {
namespace io {
  namespace pipeline {

    template < class T >
    class simple_inbound_pipeline_handler : public pipeline_handler {
    public:
      // TODO: fix cast
      void handle_event(mut< handler_context > cx,
                        own< message_read > data) override {
        auto message = fast_cast< T >(data.msg());
        if (message) {
          data.extract_message().release();
          message_received(cx, own< T >{message});
        } else { cx->forward(move(data)); }
      }

      virtual void message_received(mut< handler_context > cx,
                                    own< T > msg) = 0;
    };
  }
}
}
