#pragma once

#include "xi/ext/configure.h"
#include "xi/async/event_handler.h"
#include "xi/io/net/socket.h"
#include "xi/io/net/endpoint.h"
#include "xi/io/net/enumerations.h"
#include "xi/io/error.h"
#include "xi/io/channel_options.h"
#include "xi/io/pipes/all.h"
#include "xi/io/channel_interface.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/basic_buffer_allocator.h"
#include "xi/io/detail/heap_buffer_storage_allocator.h"

namespace xi {
namespace io {
  namespace net {

    template < address_family af, socket_type sock, protocol proto = kNone >
    class socket_base : public xi::async::io_handler {
    public:
      virtual ~socket_base() noexcept {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
      }
    };

    enum class channel_event {
      kReadable,
      kWritable,
      kClosing,
    };

    using heap_buffer_allocator =
        basic_buffer_allocator< detail::heap_buffer_storage_allocator >;

    template < address_family af, protocol proto = kNone >
    class client_channel : public socket_base< af, kStream, proto >,
                           public stream_client_socket,
                           public channel_interface {
      using endpoint_t = endpoint< af >;

      pipes::pipe< channel_event, pipes::in< error_code > > _pipe;
      endpoint_t _remote;
      own< buffer_allocator > _alloc = make< heap_buffer_allocator >();

    public:
      class data_sink;
      class data_source;

      client_channel(stream_client_socket s);

      auto pipe() {
        return edit(_pipe);
      }

    public:
      void close() override {
        this->defer([&] { _pipe.write(channel_event::kClosing); });
      }

    protected:
      void handle_read() override {
        _pipe.read(channel_event::kReadable);
      }
      void handle_write() override {
        _pipe.read(channel_event::kWritable);
      }
      own< buffer_allocator > alloc() {
        return share(_alloc);
      }
      auto pipeline() {
        return edit(_pipe);
      }
    };

    template < address_family af, protocol proto >
    struct client_channel< af, proto >::data_sink
        : public pipes::filter< channel_event, pipes::in< error_code >,
                                mut< buffer > > {
      using channel = client_channel< af, proto >;
      mut< channel > _channel;
      buffer _read_buf, _write_buf;
      data_sink(mut< channel > c) : _channel(c) {
      }

      void read(mut< context > cx, channel_event e) override {
        switch (e) {
          case channel_event::kReadable: {
            auto b = _channel->alloc()->allocate(1 << 20);
            auto ret = _channel->read(edit(b));
            _read_buf.push_back(move(b));
            if (ret.has_error()) {
              if (ret.error() == error::kEOF) {
                _channel->close();
              } else {
                cx->forward_read(ret.error());
              }
            } else {
              cx->forward_read(edit(_read_buf));
            }
          } break;
          case channel_event::kWritable: {
            if (!_write_buf.empty()) {
              _channel->write(edit(_write_buf));
              std::cout << "write queue: " << _write_buf.size() << std::endl;
            }
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
      void write(mut< context > cx, mut< buffer > b) override {
        if (_write_buf.empty()) {
          _channel->write(b);
          if (!b->size()) {
            return;
          }
        }
        _write_buf.push_back(b->split(b->size()));
        std::cout << "write queue: " << _write_buf.size() << std::endl;
        if (_write_buf.size() > 100 << 20) { // 100 MiB
          std::cout << "Closing connection due to slow reader" << std::endl;
          _channel->close();
        }
      }
    };

    template < address_family af, protocol proto >
    client_channel< af, proto >::client_channel(stream_client_socket s)
        : stream_client_socket(move(s)), _pipe(this) {
      xi::async::io_handler::descriptor(native_handle());
      _pipe.push_back(make< data_sink >(this));
    }

    template < address_family af, protocol proto = kNone >
    struct channel_factory : public virtual ownership::std_shared {
      using channel_t = client_channel< af, proto >;
      using endpoint_t = endpoint< af >;

      virtual ~channel_factory() = default;
      virtual own< channel_t > create_channel(stream_client_socket) = 0;
    };

    template < address_family af, protocol proto = kNone >
    class acceptor final : public socket_base< af, kStream, proto >,
                           public stream_server_socket,
                           public channel_interface {
      using endpoint_type = endpoint< af >;
      using client_channel_type = client_channel< af, proto >;

      pipes::pipe< pipes::in< own< client_channel_type > >,
                   pipes::in< error_code >, pipes::in< exception_ptr > > _pipe{
          this};

      own< channel_factory< af, proto > > _channel_factory;

    public:
      acceptor() : stream_server_socket(af, kStream, proto) {
        xi::async::io_handler::descriptor(native_handle());
      }

      auto pipe() {
        return edit(_pipe);
      }

      void set_channel_factory(own< channel_factory< af, proto > > f) {
        _channel_factory = move(f);
      }

    private:
      void handle_write() final override {
      }

      void close() override {
        this->cancel();
      }

      void handle_read() final override {
        endpoint_type remote;
        auto client = accept();
        if (client.has_error()) {
          _pipe.read(client.error());
        } else {
          auto socket = client.move_value();
          try {
            own< client_channel_type > ch;
            if (is_valid(_channel_factory)) {
              ch = _channel_factory->create_channel(move(socket));
            } else {
              ch = make< client_channel_type >(move(socket));
            }
            _pipe.read(move(ch));
          } catch (...) {
            _pipe.read(current_exception());
          }
        }
      }
    };
  }
}
}
