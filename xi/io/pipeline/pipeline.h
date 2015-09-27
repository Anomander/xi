#pragma once

#include "xi/ext/configure.h"
#include "xi/io/channel.h"
#include "xi/io/data_message.h"
#include "xi/io/pipeline/pipeline_handler.h"
#include "xi/io/pipeline/handler_context.h"
#include "xi/io/pipeline/pipeline_handler.hpp"

namespace xi {
namespace io {
  namespace pipeline {

    class channel;

    class pipeline : virtual public ownership::std_shared {
    public:
      pipeline(mut< channel >);

      template < class event > void fire(event &&e) {
        _fire(val(e), forward< event >(e));
      }

      void push_back(own< pipeline_handler >);
      void push_front(own< pipeline_handler >);

    private:
      template < class event > void _fire(ref< upstream_event >, event &&e) {
        _head.fire(forward< event >(e));
      }
      template < class event > void _fire(ref< downstream_event >, event &&e) {
        _tail.fire(forward< event >(e));
      }

    private:
      mut< channel > _channel;
      handler_context _head;
      handler_context _tail;

    private:
      deque< own< handler_context > > _handler_contexts{};
    };
  }
}
} // namespace pipeline // namespace io // namespace xi
