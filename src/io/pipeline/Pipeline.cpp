#include "io/pipeline/Pipeline.h"
#include "io/pipeline/Channel.h"
#include "io/DataMessage.h"

#include "ext/FastCast.h"

namespace xi {
namespace io {
  namespace pipeline {

    class HeadPipelineHandler : public PipelineHandler {
      void handleEvent(mut< HandlerContext > cx, MessageWrite ev) override {
        auto msg = fast_cast< DataMessage >(ev.message());
        if (msg) {
          // TODO: temporary fix until a proper consuming cast is implemented
          ev.extractMessage().release();
          static_cast< Channel* >(cx->channel())->doWrite(own< DataMessage >(msg));
        } else {
          throw std::logic_error("Attempt to write non-packet data");
        }
      }

      void handleEvent(mut< HandlerContext > cx, ChannelClosed ev) override {
        std::cout << "HeadPipelineHandler::channelClosed" << std::endl;
        static_cast< Channel* >(cx->channel())->doClose();
      }

      void handleEvent(mut< HandlerContext > cx, ChannelException ev) override {
        static_cast< Channel* >(cx->channel())->doClose();
        rethrow_exception(ev.exception());
      }
    };

    class TailPipelineHandler : public PipelineHandler {
      void handleEvent(mut< HandlerContext > cx, own< MessageRead > data) override {
        throw std::logic_error("Read reached tail");
      }

      void handleEvent(mut< HandlerContext > cx, ChannelError ev) override {
        // Unhandled error, rethrow as exception
        cx->fire(ChannelException(make_exception_ptr< system_error >(ev.error())));
      }
    };

    Pipeline::Pipeline(mut< Channel > channel)
        : _channel(channel)
        , _head{_channel, make< HeadPipelineHandler >()}
        , _tail{_channel, make< TailPipelineHandler >()} {
      _head.addAfter(&_tail);
    }

    void Pipeline::pushBack(own< PipelineHandler > handler) {
      auto handlerContext = make< HandlerContext >(_channel, move(handler));
      _tail.addBefore(handlerContext.get());
      _handlerContexts.emplace_back(move(handlerContext));
    }

    void Pipeline::pushFront(own< PipelineHandler > handler) {
      auto handlerContext = make< HandlerContext >(_channel, move(handler));
      _head.addAfter(handlerContext.get());
      _handlerContexts.emplace_front(move(handlerContext));
    }
  }
}
}
