#include "xi/io/pipeline/pipeline.h"
#include "xi/io/pipeline/channel.h"
#include "xi/io/data_message.h"
#include "xi/ext/fast_cast.h"

namespace xi {
namespace io {
  namespace pipeline {

    class head_pipeline_handler : public pipeline_handler {
      void handle_event(mut< handler_context > cx, message_write ev) override {
        auto msg = fast_cast< data_message >(ev.msg());
        if (msg) {
          auto data = msg->data();
          static_cast< channel * >(cx->get_channel())->do_write(byte_range{
              (uint8_t *)data, data->header().size + sizeof(protocol_message)});
        } else { throw std::logic_error("Attempt to write non-packet data"); }
      }

      void handle_event(mut< handler_context > cx, channel_closed ev) override {
        std::cout << "Head_pipeline_handler::channel_closed" << std::endl;
        static_cast< channel * >(cx->get_channel())->do_close();
      }

      void handle_event(mut< handler_context > cx,
                        channel_exception ev) override {
        static_cast< channel * >(cx->get_channel())->do_close();
        rethrow_exception(ev.exception());
      }
    };

    class tail_pipeline_handler : public pipeline_handler {
      void handle_event(mut< handler_context > cx,
                        own< message_read > data) override {
        throw std::logic_error("Read reached tail");
      }

      void handle_event(mut< handler_context > cx, channel_error ev) override {
        // unhandled error, rethrow as exception
        cx->fire(
            channel_exception(make_exception_ptr< system_error >(ev.error())));
      }
    };

    pipeline::pipeline(mut< channel > channel)
        : _channel(channel)
        , _head{_channel, make< head_pipeline_handler >()}
        , _tail{_channel, make< tail_pipeline_handler >()} {
      _head.add_after(&_tail);
    }

    void pipeline::push_back(own< pipeline_handler > handler) {
      auto hc = make< handler_context >(_channel, move(handler));
      _tail.add_before(hc.get());
      _handler_contexts.emplace_back(move(hc));
    }

    void pipeline::push_front(own< pipeline_handler > handler) {
      auto hc = make< handler_context >(_channel, move(handler));
      _head.add_after(hc.get());
      _handler_contexts.emplace_front(move(hc));
    }
  }
}
}
