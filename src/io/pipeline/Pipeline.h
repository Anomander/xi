#pragma once

#include "ext/Configure.h"
#include "io/Channel.h"
#include "io/DataMessage.h"
#include "io/pipeline/PipelineHandler.h"
#include "io/pipeline/HandlerContext.h"
#include "io/pipeline/PipelineHandler.inc"

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
      void channelOpened() { fire(ChannelOpened()); }
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
