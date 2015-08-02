#pragma once

#include "ext/Configure.h"
#include "io/Channel.h"
#include "io/DataMessage.h"
#include "io/pipeline/PipelineHandler.h"

namespace xi {
namespace io {
  namespace pipeline {

    class HandlerContext : virtual public ownership::StdShared {
    public:
      HandlerContext(mut< Channel > channel, own< PipelineHandler > handler)
          : _channel(channel), _handler(move(handler)) {
      }

      template < class Event >
      void fire(Event e) {
        _handler->onEvent< Event >(this, move(e));
      }

      template < class Event >
      void forward(Event e) {
        _forward(edit(e), move(e));
      }

      void addBefore(mut< HandlerContext > ctx) {
        if (_nextWrite) {
          ctx->_nextWrite = _nextWrite;
          _nextWrite->_prevWrite = ctx;
        }
        if (_prevRead) {
          ctx->_prevRead = _prevRead;
          _prevRead->_nextRead = ctx;
        }
        _nextWrite = ctx;
        _prevRead = ctx;
      }

      void addAfter(mut< HandlerContext > ctx) {
        if (_nextRead) {
          ctx->_nextRead = _nextRead;
          _nextRead->_prevRead = ctx;
        }
        if (_prevWrite) {
          ctx->_prevWrite = _prevWrite;
          _prevWrite->_nextWrite = ctx;
        }
        _nextRead = ctx;
        ctx->_prevRead = this;
        _prevWrite = ctx;
        ctx->_nextWrite = this;
      }

      mut< Channel > channel() noexcept {
        return _channel;
      }

    private:
      template < class Event >
      void _forward(mut< UpstreamEvent >, Event e) {
        if (_nextRead) {
          _nextRead->fire(move(e));
        }
      }
      template < class Event >
      void _forward(mut< DownstreamEvent >, Event e) {
        if (_nextWrite) {
          _nextWrite->fire(move(e));
        }
      }

    private:
      mut< Channel > _channel;
      own< PipelineHandler > _handler;

    private:
      friend class Pipeline;
      mut< HandlerContext > _prevRead{nullptr};
      mut< HandlerContext > _nextRead{nullptr};
      mut< HandlerContext > _prevWrite{nullptr};
      mut< HandlerContext > _nextWrite{nullptr};
    };
  }
}
} // namespace pipeline // namespace io // namespace xi
