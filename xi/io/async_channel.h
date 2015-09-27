#pragma once

#include "xi/ext/configure.h"
#include "xi/async/event_handler.h"
#include "xi/io/detail/socket_ops.h"
#include "xi/io/endpoint.h"
#include "xi/io/enumerations.h"
#include "xi/io/error.h"
#include "xi/io/channel.h"
#include "xi/io/data_message.h"
#include "xi/io/stream_buffer.h"
#include "xi/io/channel_options.h"
#include "xi/io/pipeline/pipeline.h"
#include "xi/io/pipeline/channel.h"
#include "xi/io/pipeline2/pipeline.h"

namespace xi {
namespace io {

  class async_channel : public xi::async::io_handler, public pipeline::channel {
  public:
    async_channel(int descriptor)
        : io_handler(descriptor), _descriptor(descriptor) {}

    virtual ~async_channel() noexcept {
      std::cout << __PRETTY_FUNCTION__ << std::endl;
      detail::socket::close(descriptor());
    }

    void attach_reactor(mut< xi::async::reactor > loop) override {
      io_handler::attach_reactor(loop);
      get_pipeline()->fire(pipeline::channel_registered());
    }

    void detach_reactor() override {
      std::cout << __PRETTY_FUNCTION__ << std::endl;
      get_pipeline()->fire(pipeline::channel_deregistered());
      io_handler::detach_reactor();
    }

    void do_close() override { io_handler::cancel(); }

    bool is_closed() override { return !is_active(); }

    template < class O > void set_option(O option) {
      auto ret = detail::socket::set_option(this->descriptor(), option);
      if (ret.has_error()) { throw system_error(ret.error()); }
    }

  protected:
    int descriptor() const noexcept { return _descriptor; }

  private:
    int _descriptor = -1;
  };

  template < address_family af, socket_type sock, protocol proto = kNone >
  class socket_base : public xi::async::io_handler {
    int _descriptor = -1;

  protected:
    int descriptor() const noexcept { return _descriptor; }

  public:
    socket_base()
        : socket_base(detail::socket::create(af, sock, proto)
                          .value()) /* will throw on failure */ {}
    socket_base(int descriptor)
        : io_handler(descriptor), _descriptor(descriptor) {}
    virtual ~socket_base() noexcept {
      std::cout << __PRETTY_FUNCTION__ << std::endl;
      detail::socket::close(descriptor());
    }
  };

  template < address_family af, socket_type sock, protocol proto = kNone >
  class channel_base : public async_channel {
  public:
    channel_base()
        : async_channel(detail::socket::create(af, sock, proto)
                            .value()) /* will throw on failure */ {}
    channel_base(int descriptor) : async_channel(descriptor) {}
  };

  template < address_family af, protocol proto = kNone >
  class client_channel : public channel_base< af, kStream, proto > {
    stream_buffer _stream_buffer;

  public:
    using endpoint_t = endpoint< af >;

  public:
    client_channel(int descriptor, endpoint_t remote)
        : channel_base< af, kStream, proto >(descriptor)
        , _remote(move(remote)) {}

  public:
    size_t read(byte_range range) override { return read({range}); }

    size_t read(initializer_list< byte_range > range) override {
      auto read = detail::socket::readv(this->descriptor(), range);
      if (XI_UNLIKELY(read.has_error())) {
        if (!is_retriable_error(read.error())) { process_error(read.error()); }
        return 0;
      }
      return read;
    }

    void do_write(byte_range range) override {
      if (_stream_buffer.empty()) {
        auto written = detail::socket::write(this->descriptor(), range);
        if (XI_UNLIKELY(written.has_error())) {
          if (is_retriable_error(written.error())) {
            _stream_buffer.push(range);
          } else { process_error(written.error()); }
          return;
        }
        if (XI_UNLIKELY(static_cast< size_t >(written) < range.size)) {
          range.consume(written);
          this->expect_write(true);
          _stream_buffer.push(range);
        }
      } else { _stream_buffer.push(range); }
    }

    void handle_read() override {
      this->get_pipeline()->fire(pipeline::data_available{});
    }

    void handle_write() override {
      if (!this->is_active()) return;
      if (_stream_buffer.empty()) {
        this->expect_write(false);
        return;
      }
      auto written =
          detail::socket::write(this->descriptor(), val(_stream_buffer));
      if (XI_UNLIKELY(written.has_error())) {
        if (!is_retriable_error(written.error())) {
          process_error(written.error());
        }
        return;
      }
      _stream_buffer.consume(written);
    }

  private:
    bool is_retriable_error(error_code error) {
      static const error_code EAgain =
          make_error_code(std::errc::resource_unavailable_try_again);
      static const error_code EWould_block =
          make_error_code(std::errc::operation_would_block);
      return (error == EAgain || error == EWould_block);
    }
    void process_error(error_code error) {
      if (error == io::error::kEOF) {
        this->close();
        return;
      }
      this->get_pipeline()->fire(pipeline::channel_error(error));
    }

  private:
    endpoint_t _remote;
    uint8_t *_current_message = nullptr;
    uint8_t *_message_cursor = nullptr;
    size_t _remaining_size = 0UL;
  };

  struct socket_readable {};
  struct socket_writable {};

  template < address_family af, protocol proto = kNone >
  class client_channel2 : public socket_base< af, kStream, proto > {
    using endpoint_t = endpoint< af >;

    pipe2::pipe< pipe2::read_only< socket_readable >,
                 pipe2::read_only< socket_writable >,
                 pipe2::read_only< error_code > > _pipe;
    endpoint_t _remote;

  public:
    client_channel2(int descriptor, endpoint_t remote)
        : socket_base< af, kStream, proto >(descriptor)
        , _remote(move(remote)) {}

    auto pipe() { return edit(_pipe); }

  public:
    expected< int > read(byte_range range) { return read({range}); }
    expected< int > read(initializer_list< byte_range > range) {
      return detail::socket::readv(this->descriptor(), range);
    }
    expected< int > write(byte_range range) {
      return detail::socket::write(this->descriptor(), range);
    }

    void close() { this->cancel(); }

  protected:
    void handle_read() override { _pipe.read(socket_readable{}); }
    void handle_write() override { _pipe.read(socket_writable{}); }
  };

  template < address_family af, protocol proto = kNone >
  class acceptor final : public socket_base< af, kStream, proto > {
    using endpoint_t = endpoint< af >;
    using client_channel_t = client_channel2< af, proto >;

    pipe2::pipe< pipe2::read_only< own< client_channel_t > >,
                 pipe2::read_only< error_code >,
                 pipe2::read_only< exception_ptr > > _pipe;

  public:
    void bind(endpoint_t ep) {
      auto ret = detail::socket::bind(this->descriptor(), ep);

      if (ret.has_error()) { throw system_error(ret.error()); }
    }

    auto pipe() { return edit(_pipe); }

  private:
    void handle_write() final override {}
    void handle_read() final override {
      endpoint_t remote;
      auto i = detail::socket::accept(this->descriptor(), edit(remote));
      if (i.has_error()) { _pipe.read(i.error()); } else {
        try {
          _pipe.read(make< client_channel_t >(i, move(remote)));
        } catch (...) { _pipe.read(current_exception()); }
      }
    }
  };

  template < address_family af, protocol proto = kNone >
  class server_channel : public channel_base< af, kStream, proto > {
  public:
    using endpoint_t = endpoint< af >;
    using client_channel_t = client_channel< af, proto >;

  private:
    using client_channel_handler = function< void(own< client_channel_t >)>;

    client_channel_handler _child_handler;

  public:
    void bind(endpoint_t ep) {
      auto ret = detail::socket::bind(this->descriptor(), ep);

      if (ret.has_error()) { throw system_error(ret.error()); }
    }
    void child_handler(client_channel_handler handler) {
      _child_handler = move(handler);
    }

  private:
    void do_write(byte_range) final override {}
    void handle_write() final override {}
    size_t read(byte_range range) final override { return 0; }
    size_t read(initializer_list< byte_range > range) final override {
      return 0;
    }

    void handle_read() override {
      endpoint_t remote;
      auto i = detail::socket::accept(this->descriptor(), edit(remote));
      if (i.has_error()) {
        std::cout << i.error().message() << std::endl;
        return;
      }
      // int T = 1;
      // setsockopt(i, IPPROTO_TCP, TCP_NODELAY, &T, sizeof(T));
      // auto child = make< client_channel_t >(i, move(remote));
      // this->get_pipeline()->channel_read(make< client_channel_connected
      // >(move(child)));
      if (_child_handler) {
        _child_handler(make< client_channel_t >(i, move(remote)));
      }
    }
  };

  template < address_family af, protocol proto = kNone >
  class endpoint_channel : public channel_base< af, kDatagram, proto > {
  public:
    using endpoint_t = endpoint< af >;

  public:
    void handle_read() override;
    void handle_write() override;
  };
}
}
