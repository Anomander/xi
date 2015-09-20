#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/Channel.h"
#include "xi/io/DataMessage.h"
#include "xi/io/pipeline/PipelineHandler.h"
#include "xi/io/pipeline/HandlerContext.h"
#include "xi/io/pipeline/PipelineHandler.hpp"

namespace xi {
namespace io {
  namespace pipeline {

    class Channel;

    class Pipeline : virtual public ownership::StdShared {
    public:
      Pipeline(mut< Channel >);

      template < class Event >
      void fire(Event&& e) {
        _fire(val(e), forward< Event >(e));
      }

      void pushBack(own< PipelineHandler >);
      void pushFront(own< PipelineHandler >);

      void channelRead(own< Message > msg) { fire(MessageRead(move(msg))); }
      void channelWrite(own< Message > msg) { fire(MessageWrite(move(msg))); }
      void channelRegistered() { fire(ChannelRegistered()); }
      void channelDeregistered() { fire(ChannelDeregistered()); }
      void channelClosed() { fire(ChannelClosed()); }
      void channelError(error_code error) { fire(ChannelError(error)); }
      void channelException(exception_ptr ex) { fire(ChannelException(ex)); }

    private:
      template < class Event >
      void _fire(ref< UpstreamEvent >, Event&& e) {
        _head.fire(forward< Event >(e));
      }
      template < class Event >
      void _fire(ref< DownstreamEvent >, Event&& e) {
        _tail.fire(forward< Event >(e));
      }

    private:
      mut< Channel > _channel;
      HandlerContext _head;
      HandlerContext _tail;

    private:
      deque< own< HandlerContext > > _handlerContexts{};
    };
  }
}
} // namespace pipeline // namespace io // namespace xi
