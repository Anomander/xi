#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipeline/events.h"

namespace xi {
namespace io {
  namespace pipeline {

    class handler_context;

    template < class E > struct event_handler_base {
      virtual void handle_event(mut< handler_context > cx, own< E > e);

    protected:
      void on_event(mut< handler_context > cx, own< E > e) {
        this->handle_event(cx, std::move(e));
      }
    };

    class pipeline_handler
        : virtual public ownership::std_shared,
          protected meta::multi_inherit_template<
              event_handler_base, message_read, message_write,
              channel_registered, channel_deregistered, channel_opened,
              channel_closed, channel_error, channel_exception, data_available,
              user_upstream_event, user_downstream_event > {
    public:
      template < class event >
      void on_event(mut< handler_context > cx, own< event > e) {
        event_handler_base< event >::on_event(cx, std::move(e));
      }
    };
  }
}
}
