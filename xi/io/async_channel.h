#pragma once

#include "xi/ext/configure.h"
#include "xi/async/event_handler.h"
#include "xi/io/detail/socket_ops.h"
#include "xi/io/endpoint.h"
#include "xi/io/enumerations.h"
#include "xi/io/error.h"
#include "xi/io/data_message.h"
#include "xi/io/stream_buffer.h"
#include "xi/io/channel_options.h"
#include "xi/io/pipes/all.h"

namespace xi {
namespace io {

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

  struct socket_readable {};
  struct socket_writable {};

  template < address_family af, protocol proto = kNone >
  class client_channel2 : public socket_base< af, kStream, proto > {
    using endpoint_t = endpoint< af >;

    pipes::pipe< pipes::read_only< socket_readable >,
                 pipes::read_only< socket_writable >,
                 pipes::read_only< error_code > > _pipe;
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
    using endpoint_type = endpoint< af >;
    using client_channel_type = client_channel2< af, proto >;

    pipes::pipe< pipes::read_only< own< client_channel_type > >,
                 pipes::read_only< error_code >,
                 pipes::read_only< exception_ptr > > _pipe;

  public:
    void bind(endpoint_type ep) {
      auto ret = detail::socket::bind(this->descriptor(), ep);

      if (ret.has_error()) { throw system_error(ret.error()); }
    }

    auto pipe() { return edit(_pipe); }

  private:
    void handle_write() final override {}
    void handle_read() final override {
      endpoint_type remote;
      auto i = detail::socket::accept(this->descriptor(), edit(remote));
      if (i.has_error()) { _pipe.read(i.error()); } else {
        try {
          _pipe.read(make< client_channel_type >(i, move(remote)));
        } catch (...) { _pipe.read(current_exception()); }
      }
    }
  };

}
}
