#include "io/pipeline/Pipeline.h"
#include "io/DataMessage.h"

#include "ext/FastCast.h"

namespace xi {
  namespace io {
    namespace pipeline {

      class HeadPipelineHandler : public pipeline::PipelineHandler {
        void channelWrite (mut<HandlerContext> cx, own<Message> data) override {
          // auto packet = dynamic_pointer_cast<Packet> (move(data));
          // if (packet) {
          //     ChannelPrivateInterface::doWrite (cx->channel(), move(packet));
          // } else {
          //     throw std::logic_error("Attempt to write non-packet data");
          // }
          auto msg = fast_cast<DataMessage> (data.get());
          if (msg) {
            data.release();
            static_cast<pipeline::Channel *>(cx->channel())->doWrite (own <DataMessage> (msg));
          } else {
            throw std::logic_error("Attempt to write non-packet data");
          }
        }

        void channelClosed (mut<HandlerContext> cx) {
          std::cout << "HeadPipelineHandler::channelClosed" << std::endl;
          static_cast<pipeline::Channel *>(cx->channel())->doClose ();
        }

        void channelException (mut<HandlerContext> cx, exception_ptr ex) override {
          static_cast<pipeline::Channel *>(cx->channel())->doClose ();
          rethrow_exception(ex);
        }
      };

      class TailPipelineHandler : public PipelineHandler {
        void channelRead (mut<HandlerContext> cx, own<Message> data) override {
          throw std::logic_error("Read reached tail");
        }

        void channelError (mut<HandlerContext> cx, error_code error) override {
          /// Unhandled error, rethrow as exception
          cx->channelException(make_exception_ptr<system_error>(error));
        }

      };

      Pipeline::Pipeline (Channel * channel)
        : _channel (channel)
        , _head { _channel, make <HeadPipelineHandler> () }
        , _tail { _channel, make <TailPipelineHandler> () }
{
  _head.addAfter (& _tail);
}

      void Pipeline::channelRead (own<Message> msg) { _head.channelRead (move(msg)); }
      void Pipeline::channelWrite (own<Message> msg) { _tail.channelWrite (move(msg)); }
      void Pipeline::channelRegistered () { _head.channelRegistered (); }
      void Pipeline::channelDeregistered () { _head.channelDeregistered (); }
      void Pipeline::channelOpened () { _head.channelOpened (); }
      void Pipeline::channelClosed () { _tail.channelClosed (); }
      void Pipeline::channelError (error_code error) { _head.channelError (error); }
      void Pipeline::channelException (exception_ptr ex) { _head.channelException (ex); }

      void Pipeline::pushBack (shared_ptr<PipelineHandler> handler) {
        auto handlerContext = make <HandlerContext> (_channel, move(handler));
        _tail.addBefore (handlerContext.get());
        _handlerContexts.emplace_back(move(handlerContext));
      }

      void Pipeline::pushFront (shared_ptr<PipelineHandler> handler) {
        auto handlerContext = make <HandlerContext> (_channel, move(handler));
        _head.addAfter (handlerContext.get());
        _handlerContexts.emplace_front(move(handlerContext));
      }
    }}}
