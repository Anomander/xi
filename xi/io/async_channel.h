#pragma once

#include "xi/ext/configure.h"
#include "xi/async/event_handler.h"
#include "xi/io/detail/socket_ops.h"
#include "xi/io/endpoint.h"
#include "xi/io/enumerations.h"
#include "xi/io/error.h"
#include "xi/io/data_message.h"
#include "xi/io/channel_options.h"
#include "xi/io/pipes/all.h"
#include "xi/io/channel_interface.h"
#include "xi/io/byte_buffer.h"
#include "xi/io/byte_buffer_allocator.h"
#include "xi/io/basic_byte_buffer_allocator.h"
#include "xi/io/detail/heap_byte_buffer_storage_allocator.h"

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

  enum class channel_event {
    kReadable,
    kWritable,
    kClosing,
  };

  using heap_byte_buffer_allocator =
      basic_byte_buffer_allocator< detail::heap_byte_buffer_storage_allocator >;

  template < address_family af, protocol proto = kNone >
  class client_channel2 : public socket_base< af, kStream, proto >,
                          public channel_interface {
    using endpoint_t = endpoint< af >;

    pipes::pipe< channel_event, pipes::in< error_code > > _pipe;
    endpoint_t _remote;
    own< byte_buffer_allocator > _alloc = make< heap_byte_buffer_allocator >();

  public:
    class data_sink;
    class data_source;

    client_channel2(int descriptor, endpoint_t remote);

    auto pipe() { return edit(_pipe); }

  public:
    void close() override {
      this->defer([&] { _pipe.write(channel_event::kClosing); });
    }

  protected:
    void handle_read() override { _pipe.read(channel_event::kReadable); }
    void handle_write() override { _pipe.read(channel_event::kWritable); }
    own< byte_buffer_allocator > alloc() { return share(_alloc); }
    auto pipeline() { return edit(_pipe); }
  };

  template < address_family af, protocol proto >
  struct client_channel2< af, proto >::data_sink
      : public pipes::filter< channel_event, pipes::in< error_code >,
                              own< byte_buffer > > {
    using channel = client_channel2< af, proto >;
    mut< channel > _channel;
    data_sink(mut< channel > c) : _channel(c) {}

    void read(mut< context > cx, channel_event e) override {
      switch (e) {
      case channel_event::kReadable: {
        auto b = _channel->alloc()->allocate(1 << 20);
        auto ret = detail::socket::read(_channel->descriptor(), edit(b));
        if (ret.has_error()) {
          if (ret.error() == error::kEOF) { _channel->close(); } else {
            cx->forward_read(ret.error());
          }
        } else { cx->forward_read(move(b)); }
      } break;
      default:
        break;
      };
    }
    void write(mut< context > cx, channel_event e) override {
      switch (e) {
        case channel_event::kClosing:
          _channel->cancel();
          break;
        default:
          break;
      };
    }
    void write(mut< context > cx, own< byte_buffer > b) override {
      detail::socket::write(_channel->descriptor(), edit(b));
    }
  };

  template < address_family af, protocol proto >
  client_channel2< af, proto >::client_channel2(int descriptor,
                                                endpoint_t remote)
      : socket_base< af, kStream, proto >(descriptor)
      , _pipe(this)
      , _remote(move(remote)) {
    _pipe.push_back(make< data_sink >(this));
  }

  template < address_family af, protocol proto = kNone >
  class acceptor final : public socket_base< af, kStream, proto >,
                         public channel_interface {
    using endpoint_type = endpoint< af >;
    using client_channel_type = client_channel2< af, proto >;

    pipes::pipe< pipes::in< own< client_channel_type > >,
                 pipes::in< error_code >, pipes::in< exception_ptr > > _pipe{
        this};

  public:
    void bind(endpoint_type ep) {
      auto ret = detail::socket::bind(this->descriptor(), ep);

      if (ret.has_error()) { throw system_error(ret.error()); }
    }

    auto pipe() { return edit(_pipe); }

  private:
    void handle_write() final override {}

    void close() override { this->cancel(); }

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
