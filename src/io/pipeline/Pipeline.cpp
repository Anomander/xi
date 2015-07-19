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

// Class HeadPipelineHandler : public pipeline::PipelineHandler {
//   void channelWrite (mut<HandlerContext> cx, own<Message> data) override {
//     // auto packet = dynamic_pointer_cast<Packet> (move(data));
//     // if (packet) {
//     //     ChannelPrivateInterface::doWrite (cx->channel(), move(packet));
//     // } else {
//     //     throw std::logic_error("Attempt to write non-packet data");
//     // }
//     auto msg = fast_cast<DataMessage> (data.get());
//     if (msg) {
//       data.release();
//       static_cast<pipeline::Channel *>(cx->channel())->doWrite (own
//       <DataMessage> (msg));
//     } else {
//       throw std::logic_error("Attempt to write non-packet data");
//     }
//   }

//   void channelClosed (mut<HandlerContext> cx) {
//     std::cout << "HeadPipelineHandler::channelClosed" << std::endl;
//     static_cast<pipeline::Channel *>(cx->channel())->doClose ();
//   }

//   void channelException (mut<HandlerContext> cx, exception_ptr ex) override {
//     static_cast<pipeline::Channel *>(cx->channel())->doClose ();
//     rethrow_exception(ex);
//   }
// };

// class TailPipelineHandler : public PipelineHandler {
//   void channelRead (mut<HandlerContext> cx, own<Message> data) override {
//     throw std::logic_error("Read reached tail");
//   }

//   void channelError (mut<HandlerContext> cx, error_code error) override {
//     /// Unhandled error, rethrow as exception
//     cx->channelException(make_exception_ptr<system_error>(error));
//   }

// };

Pipeline::Pipeline(mut< Channel > channel)
    : _channel(channel),
      _head{ _channel, make< HeadPipelineHandler >() },
      _tail{ _channel, make< TailPipelineHandler >() } {
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
