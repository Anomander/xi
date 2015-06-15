#pragma once

#include "ext/Configure.h"
#include "io/Channel.h"
#include "io/DataMessage.h"

namespace xi { namespace io { namespace pipeline {

      class HandlerContext;
      class Channel;

      class PipelineHandler
        : virtual public ownership::StdShared
      {
      public:
        virtual ~PipelineHandler() noexcept = default;
        virtual void channelRead(mut<HandlerContext>, own<Message>);
        virtual void channelWrite(mut<HandlerContext>, own<Message>);
        virtual void channelRegistered (mut<HandlerContext>);
        virtual void channelDeregistered (mut<HandlerContext>);
        virtual void channelOpened (mut<HandlerContext>);
        virtual void channelClosed (mut<HandlerContext>);
        virtual void channelError (mut<HandlerContext>, error_code);
        virtual void channelException (mut<HandlerContext>, exception_ptr);
      };

      class HandlerContext
        : public ownership::Unique
      {
      public:
        void channelRead(own<Message>);
        void channelWrite(own<Message>);
        void channelRegistered ();
        void channelDeregistered ();
        void channelOpened ();
        void channelClosed ();
        void channelError (error_code);
        void channelException (exception_ptr);

        mut<Channel> channel() noexcept { return _channel; }

        HandlerContext (mut<Channel> channel, own<PipelineHandler> handler)
          : _channel (channel)
          , _handler (move(handler))
        {}

      public:
        virtual ~HandlerContext() noexcept = default;
        virtual void doChannelRead(own<Message>);
        virtual void doChannelWrite(own<Message>);
        virtual void doChannelRegistered ();
        virtual void doChannelDeregistered ();
        virtual void doChannelOpened ();
        virtual void doChannelClosed ();
        virtual void doChannelError (error_code);
        virtual void doChannelException (exception_ptr);

        void addBefore(mut<HandlerContext> ctx) {
          if (_nextWrite) { ctx->_nextWrite = _nextWrite; _nextWrite->_prevWrite = ctx; }
          if (_prevRead) { ctx->_prevRead = _prevRead; _prevRead->_nextRead = ctx; }
          _nextWrite = ctx;
          _prevRead = ctx;
        }
        void addAfter(mut<HandlerContext> ctx) {
          if (_nextRead) { ctx->_nextRead = _nextRead; _nextRead->_prevRead = ctx; }
          if (_prevWrite) { ctx->_prevWrite = _prevWrite; _prevWrite->_nextWrite = ctx; }
          _nextRead = ctx; ctx->_prevRead = this;
          _prevWrite = ctx; ctx->_nextWrite = this;
        }

      private:
        mut<Channel> _channel;
        own<PipelineHandler> _handler;

      private:
        friend class Pipeline;
        mut<HandlerContext> _prevRead {nullptr};
        mut<HandlerContext> _nextRead {nullptr};
        mut<HandlerContext> _prevWrite {nullptr};
        mut<HandlerContext> _nextWrite {nullptr};
      };

      inline void HandlerContext::channelRead (own<Message> msg) {
        if (_nextRead) {
          _nextRead->doChannelRead(move(msg));
        }
      }
      inline void HandlerContext::channelWrite (own<Message> msg) {
        if (_nextWrite) {
          _nextWrite->doChannelWrite(move(msg));
        }
      }
      inline void HandlerContext::channelRegistered () {
        if (_nextRead) {
          _nextRead->doChannelRegistered();
        }
      }
      inline void HandlerContext::channelDeregistered () {
        if (_nextRead) {
          _nextRead->doChannelDeregistered();
        }
      }
      inline void HandlerContext::channelOpened () {
        if (_nextRead) {
          _nextRead->doChannelOpened();
        }
      }
      inline void HandlerContext::channelClosed () {
        if (_nextWrite) {
          _nextWrite->doChannelClosed();
        }
      }
      inline void HandlerContext::channelError (error_code error) {
        if (_nextRead) {
          _nextRead->doChannelError(error);
        }
      }
      inline void HandlerContext::channelException (exception_ptr ex) {
        if (_nextWrite) {
          _nextWrite->doChannelException(ex);
        }
      }
      inline void HandlerContext::doChannelRead (own<Message> msg) {
        _handler->channelRead (this, move(msg));
      }
      inline void HandlerContext::doChannelWrite (own<Message> msg) {
        _handler->channelWrite (this, move(msg));
      }
      inline void HandlerContext::doChannelRegistered () {
        _handler->channelRegistered(this);
      }
      inline void HandlerContext::doChannelDeregistered () {
        _handler->channelDeregistered(this);
      }
      inline void HandlerContext::doChannelOpened () {
        _handler->channelOpened(this);
      }
      inline void HandlerContext::doChannelClosed () {
        _handler->channelClosed(this);
      }
      inline void HandlerContext::doChannelError (error_code error) {
        _handler->channelError(this, error);
      }
      inline void HandlerContext::doChannelException (exception_ptr ex) {
        _handler->channelException(this, ex);
      }

      // template<class T>
      // class SimpleInboundPipelineHandler : public PipelineHandler {
      // public:
      //   void read (mut<HandlerContext> cx, own<Request> data) override {
      //     auto message = dynamic_cast<T*> (data.get());
      //     if (message) {
      //       data.release();
      //       messageReceived(cx, own <T> {message});
      //     } else {
      //       cx->read (move (data));
      //     }
      //   }
      // protected:
      //   virtual void messageReceived (mut<HandlerContext>, own <T> ) = 0;
      // };

      template<class T>
      class SimpleInboundPipelineFastCastHandler
        : public PipelineHandler
      {
      public:
        void channelRead (mut<HandlerContext> cx, own<Message> data) override {
          auto message = fast_cast<T> (data.get());
          if (message) {
            data.release();
            messageReceived(cx, own <T> {message});
          } else {
            cx->channelRead (move (data));
          }
        }
      protected:
        virtual void messageReceived (mut<HandlerContext>, own <T> ) = 0;
      };

      /**
       * @section Pipeline
       * .
       */
      class Pipeline
        : virtual public ownership::StdShared
      {
      public:
        Pipeline (mut<Channel>);

        void channelRead (own<Message>);
        void channelWrite (own<Message>);
        void channelRegistered ();
        void channelDeregistered ();
        void channelOpened ();
        void channelClosed ();
        void channelError (error_code);
        void channelException (exception_ptr);

        void pushBack (own<PipelineHandler>);
        void pushFront (own<PipelineHandler>);

      private:
        mut<Channel> _channel;
        HandlerContext _head;
        HandlerContext _tail;

      private:
        deque<own<HandlerContext>> _handlerContexts {};
      };

      class Channel
        : public io::Channel
      {
      protected:
        virtual ~Channel() noexcept = default;

      public:
        mut <Pipeline> pipeline() noexcept { return edit(_pipeline); }

      public:
        void close() final override {
          pipeline()->channelClosed();
        }
        void write(own<Message> msg) final override {
          pipeline()->channelWrite(move(msg));
        }
      protected:
        friend class HeadPipelineHandler;
        friend class TailPipelineHandler;

        virtual void doWrite (own<DataMessage>) = 0;
        virtual void doClose () = 0;

      private:
        own <Pipeline> _pipeline = make <Pipeline> ( this );
      };

    }}} // namespace pipeline // namespace io // namespace xi
