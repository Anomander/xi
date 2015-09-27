#pragma once

#include "xi/ext/configure.h"
#include "xi/io/channel.h"
#include "xi/io/data_message.h"
#include "xi/io/pipeline/pipeline_handler.h"

namespace xi {
namespace io {
  namespace pipeline {

    class handler_context : virtual public ownership::std_shared {
    public:
      handler_context(mut< channel > channel, own< pipeline_handler > handler)
          : _channel(channel), _handler(move(handler)) {}

      template < class event > void fire(event e) {
        try {
          _handler->on_event< event >(this, move(e));
        } catch (...) { forward(channel_exception{current_exception()}); }
      }

      template < class event > void forward(event e) {
        _forward(val(e), move(e));
      }

      void add_before(mut< handler_context > ctx) {
        if (_next_write) {
          ctx->_next_write = _next_write;
          _next_write->_prev_write = ctx;
        }
        if (_prev_read) {
          ctx->_prev_read = _prev_read;
          _prev_read->_next_read = ctx;
        }
        _next_write = ctx;
        _prev_read = ctx;
      }

      void add_after(mut< handler_context > ctx) {
        if (_next_read) {
          ctx->_next_read = _next_read;
          _next_read->_prev_read = ctx;
        }
        if (_prev_write) {
          ctx->_prev_write = _prev_write;
          _prev_write->_next_write = ctx;
        }
        _next_read = ctx;
        ctx->_prev_read = this;
        _prev_write = ctx;
        ctx->_next_write = this;
      }

      mut< channel > get_channel() noexcept { return _channel; }

    private:
      template < class event > void _forward(ref< upstream_event >, event e) {
        if (_next_read) { _next_read->fire(move(e)); }
      }
      template < class event > void _forward(ref< downstream_event >, event e) {
        if (_next_write) { _next_write->fire(move(e)); }
      }

    private:
      mut< channel > _channel;
      own< pipeline_handler > _handler;

    private:
      friend class pipeline;
      mut< handler_context > _prev_read{nullptr};
      mut< handler_context > _next_read{nullptr};
      mut< handler_context > _prev_write{nullptr};
      mut< handler_context > _next_write{nullptr};
    };
  }
}
} // namespace pipeline // namespace io // namespace xi
