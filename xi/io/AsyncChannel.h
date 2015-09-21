#pragma once

#include "xi/ext/Configure.h"
#include "xi/async/EventHandler.h"
#include "xi/io/detail/SocketOps.h"
#include "xi/io/Endpoint.h"
#include "xi/io/Enumerations.h"
#include "xi/io/Error.h"
#include "xi/io/Channel.h"
#include "xi/io/DataMessage.h"
#include "xi/io/StreamBuffer.h"
#include "xi/io/ChannelOptions.h"
#include "xi/io/pipeline/Pipeline.h"
#include "xi/io/pipeline/Channel.h"

namespace xi {
namespace io {

  class AsyncChannel : public async::IoHandler, public pipeline::Channel {
  public:
    AsyncChannel(int descriptor) : IoHandler(descriptor), _descriptor(descriptor) {}

    virtual ~AsyncChannel() noexcept {
      std::cout << __PRETTY_FUNCTION__ << std::endl;
      detail::socket::close(descriptor());
    }

    void attachReactor(mut< async::Reactor > loop) override {
      IoHandler::attachReactor(loop);
      pipeline()->fire(pipeline::ChannelRegistered());
    }

    void detachReactor() override {
      std::cout << __PRETTY_FUNCTION__ << std::endl;
      pipeline()->fire(pipeline::ChannelDeregistered());
      IoHandler::detachReactor();
    }

    void doClose() override { IoHandler::cancel(); }

    bool isClosed() override {
      return !isActive();
    }

    template < class Option >
    void setOption(Option option) {
      auto ret = detail::socket::setOption(this->descriptor(), option);
      if (ret.hasError()) {
        throw system_error(ret.error());
      }
    }

  protected:
    int descriptor() const noexcept { return _descriptor; }

  private:
    int _descriptor = -1;
  };

  template < AddressFamily af, SocketType sock, Protocol proto = kNone >
  class ChannelBase : public AsyncChannel {
  public:
    ChannelBase() : AsyncChannel(detail::socket::create(af, sock, proto).value()) /* will throw on failure */ {}
    ChannelBase(int descriptor) : AsyncChannel(descriptor) {}
  };

  template < AddressFamily af, Protocol proto = kNone >
  class ClientChannel : public ChannelBase< af, kStream, proto > {
    StreamBuffer _streamBuffer;

  public:
    using Endpoint_t = Endpoint< af >;

  public:
    ClientChannel(int descriptor, Endpoint_t remote)
        : ChannelBase< af, kStream, proto >(descriptor), _remote(move(remote)) {}

  public:
    size_t read(ByteRange range) override { return read({range}); }

    size_t read(initializer_list< ByteRange > range) override {
      auto read = detail::socket::readv(this->descriptor(), range);
      if (XI_UNLIKELY(read.hasError())) {
        if (!isRetriableError(read.error())) {
          processError(read.error());
        }
        return 0;
      }
      return read;
    }

    void doWrite(ByteRange range) override {
      if (_streamBuffer.empty()) {
        auto written = detail::socket::write(this->descriptor(), range);
        if (XI_UNLIKELY(written.hasError())) {
          if (isRetriableError(written.error())) {
            _streamBuffer.push(range);
          } else {
            processError(written.error());
          }
          return;
        }
        if (XI_UNLIKELY(static_cast< size_t >(written) < range.size)) {
          range.consume(written);
          this->expectWrite(true);
          _streamBuffer.push(range);
        }
      } else {
        _streamBuffer.push(range);
      }
    }

    void handleRead() override { this->pipeline()->fire(pipeline::DataAvailable{}); }

    void handleWrite() override {
      if (!this->isActive())
        return;
      if (_streamBuffer.empty()) {
        this->expectWrite(false);
        return;
      }
      auto written = detail::socket::write(this->descriptor(), val(_streamBuffer));
      if (XI_UNLIKELY(written.hasError())) {
        if (!isRetriableError(written.error())) {
          processError(written.error());
        }
        return;
      }
      _streamBuffer.consume(written);
    }

  private:
    bool isRetriableError(error_code error) {
      static const error_code EAgain = make_error_code(SystemError::resource_unavailable_try_again);
      static const error_code EWouldBlock = make_error_code(SystemError::operation_would_block);
      return (error == EAgain || error == EWouldBlock);
    }
    void processError(error_code error) {
      if (error == io::error::kEof) {
        this->close();
        return;
      }
      this->pipeline()->fire(pipeline::ChannelError(error));
    }

  private:
    Endpoint_t _remote;
    uint8_t* _currentMessage = nullptr;
    uint8_t* _messageCursor = nullptr;
    size_t _remainingSize = 0UL;
  };

  template < AddressFamily af, Protocol proto = kNone >
  class ServerChannel : public ChannelBase< af, kStream, proto > {
  public:
    using Endpoint_t = Endpoint< af >;
    using ClientChannel_t = ClientChannel< af, proto >;

  private:
    using ClientChannelHandler = function< void(own< ClientChannel_t >)>;

    ClientChannelHandler _childHandler;

  public:
    void bind(Endpoint_t ep) {
      auto ret = detail::socket::bind(this->descriptor(), ep);

      if (ret.hasError()) {
        throw system_error(ret.error());
      }
    }
    void childHandler(ClientChannelHandler handler) { _childHandler = move(handler); }

  private:
    void doWrite(ByteRange) final override {}
    void handleWrite() final override {}
    size_t read(ByteRange range) final override { return 0; }
    size_t read(initializer_list< ByteRange > range) final override { return 0; }

    void handleRead() override {
      Endpoint_t remote;
      auto i = detail::socket::accept(this->descriptor(), edit(remote));
      if (i.hasError()) {
        std::cout << i.error().message() << std::endl;
      }
      // int T = 1;
      // setsockopt(i, IPPROTO_TCP, TCP_NODELAY, &T, sizeof(T));
      // auto child = make< ClientChannel_t >(i, move(remote));
      // this->pipeline()->channelRead(make< ClientChannelConnected >(move(child)));
      if (_childHandler) {
        _childHandler(make< ClientChannel_t >(i, move(remote)));
      }
    }
  };

  template < AddressFamily af, Protocol proto = kNone >
  class EndpointChannel : public ChannelBase< af, kDatagram, proto > {
  public:
    using Endpoint_t = Endpoint< af >;

  public:
    void handleRead() override;
    void handleWrite() override;
  };
}
}
