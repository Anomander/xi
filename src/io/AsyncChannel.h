#pragma once

#include "ext/Configure.h"
#include "async/EventHandler.h"
#include "io/detail/SocketOps.h"
#include "io/Endpoint.h"
#include "io/Enumerations.h"
#include "io/Error.h"
#include "io/Channel.h"
#include "io/DataMessage.h"
#include "io/ChannelOptions.h"
#include "io/pipeline/Pipeline.h"

namespace xi {
  namespace io {

    class AsyncChannel
      : public async::IoHandler
      , public pipeline::Channel
    {
    public:
      AsyncChannel (Expected<int> descriptor)
        : IoHandler (descriptor.value()) // will throw on error
        , _descriptor (descriptor.value())
      {}

      virtual ~AsyncChannel() noexcept {
        detail::socket::close(descriptor());
      }

      void attachExecutor (mut <async::Executor> loop) override {
        IoHandler::attachExecutor (loop);
        pipeline()->channelRegistered();
      }

      void detachExecutor () override {
        IoHandler::detachExecutor ();
        pipeline()->channelDeregistered();
      }

      void doClose() override {
        IoHandler::remove();
      }

      template <class Option>
      void setOption(Option option) {
        auto ret = detail::socket::setOption (this->descriptor(), option);
        if (ret.hasError()) {
          throw system_error (ret.error());
        }
      }

    protected:
      int descriptor() const noexcept { return _descriptor; }

    private:
      int _descriptor = -1;
    };

    template <AddressFamily af, SocketType sock, Protocol proto = kNone>
    class ChannelBase
      : public AsyncChannel
    {
    public:
      ChannelBase ()
        : AsyncChannel (detail::socket::create(af, sock, proto))
      {}
      ChannelBase (int descriptor)
        : AsyncChannel (descriptor)
      {}

    public:
      virtual void open() {
        std::cout << "ChannelBase::open" << std::endl;
        this->pipeline()->channelOpened();
      }
    };

    template <AddressFamily af, Protocol proto = kNone>
    class ClientChannel
      : public ChannelBase <af, kStream, proto>
    {
    public:
      using Endpoint_t = Endpoint <af>;

    public:
      ClientChannel(int descriptor, Endpoint_t remote)
        : ChannelBase <af, kStream, proto> (descriptor)
        , _remote (move(remote))
      {}
    public:
      void doWrite(own<DataMessage> msg) override {
        static const error_code EAgain = make_error_code (SystemError::resource_unavailable_try_again);
        static const error_code EWouldBlock = make_error_code (SystemError::operation_would_block);

        // std::cout << "Message written" << std::endl;
        auto data = msg->data();
        auto written = detail::socket::write (this->descriptor(), data, data->header().size + sizeof(ProtocolMessage));
        if (written.hasError()) {
          auto error = written.error();
          if (error == EAgain || error == EWouldBlock) {
            std::cout << "Write delayed." << std::endl;
            return;
          }
          if (error == io::error::kEof) {
            this->close();
            return;
          }
          std::cout << "Error on write." << std::endl;
          this->pipeline()->channelError(error);
          return;
        }
      }
      void handleRead() override {
        static const error_code EAgain = make_error_code (SystemError::resource_unavailable_try_again);
        static const error_code EWouldBlock = make_error_code (SystemError::operation_would_block);

        auto destructionGuard = this->self();
        while (this->isActive()) {
          if (_currentMessage) {
            // std::cout << "Message present: " << _remainingSize << " bytes needed." << std::endl;
            auto read = detail::socket::read (this->descriptor(), _messageCursor, _remainingSize);
            if (read.hasError()) {
              auto error = read.error();
              if (error == EAgain || error == EWouldBlock) {
                break;
              }
              if (error == io::error::kEof) {
                this->close();
                return;
              }
              this->pipeline()->channelError(error);
              return;
            }
            // std::cout << "Read " << read << " bytes." << std::endl;
            if (0 == (_remainingSize -= read)) {
              auto * message = _currentMessage;
              _messageCursor = _currentMessage = nullptr;
              this->pipeline()->channelRead(make <DataMessage> (new (message) ProtocolMessage));
            } else {
              _messageCursor += read;
            }
          } else {
            ProtocolHeader hdr;
            auto sz = detail::socket::peek (this->descriptor(), &hdr, sizeof(hdr));
            if (sz.hasError()) {
              auto error = sz.error();
              if (error == EAgain || error == EWouldBlock) {
                break;
              }
              if (error == io::error::kEof) {
                this->close();
                return;
              }
              this->pipeline()->channelError(error);
              return;
            }
            // std::cout << "Readable " << sz << " bytes." << std::endl;
            if ((size_t)sz >= sizeof (hdr)) {
              // std::cout << "Message size: " << hdr.size << std::endl;
              _remainingSize = sizeof(ProtocolMessage) + hdr.size;
              _messageCursor = _currentMessage = (uint8_t*)::malloc (_remainingSize);
            } else {
              break;
            }
          }
        }
      }
      void handleWrite() override {
        if (! this->isActive()) return;
        // std::cout << "Writable" << std::endl;
      }

    private:
      Endpoint_t _remote;
      uint8_t * _currentMessage = nullptr;
      uint8_t * _messageCursor = nullptr;
      size_t _remainingSize = 0UL;
    };

    template <AddressFamily af, Protocol proto = kNone>
    class ServerChannel
      : public ChannelBase <af, kStream, proto>
    {
    public:
      using Endpoint_t = Endpoint <af>;
      using ClientChannel_t = ClientChannel <af, proto>;
      using ChildHandler_t = function <void(own<ClientChannel_t>)>;

    public:
      void bind(Endpoint_t ep) {
        auto ret = detail::socket::bind(this->descriptor(), ep);

        if (ret.hasError()) {
          throw system_error (ret.error());
        }
      }

      void childHandler (ChildHandler_t h) { _childHandler = move(h); }

    private:
      void doWrite(own<DataMessage>) final override {}
      void handleWrite() final override {}
      void handleRead() override {
        Endpoint_t remote;
        auto i = detail::socket::accept (this->descriptor(), edit(remote));
        if (i.hasError()) {
          std::cout << i.error().message() << std::endl;
        }
        // int T = 1;
        // setsockopt(i, IPPROTO_TCP, TCP_NODELAY, &T, sizeof(T));
        auto child = make <ClientChannel_t> (i, move(remote));
        child->open();
        _childHandler (move(child));
      }

    private:
      ChildHandler_t _childHandler;
    };

    template <AddressFamily af, Protocol proto = kNone>
    class EndpointChannel
      : public ChannelBase <af, kDatagram, proto>
    {
    public:
      using Endpoint_t = Endpoint <af>;

    public:
      void handleRead() override;
      void handleWrite() override;
    };
  }
}
