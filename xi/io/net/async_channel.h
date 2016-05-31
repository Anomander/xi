#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/coroutine.h"
#include "xi/core/async_defer.h"
#include "xi/core/bootstrap.h"
#include "xi/core/event_handler.h"
#include "xi/core/future.h"
#include "xi/io/basic_buffer_allocator.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/channel_interface.h"
#include "xi/io/error.h"
#include "xi/io/net/channel_options.h"
#include "xi/io/net/endpoint.h"
#include "xi/io/net/enumerations.h"
#include "xi/io/net/socket2.h"
#include "xi/io/pipes/all.h"

namespace xi {
namespace io {
  namespace net {

    enum class socket_event {
      kReadable,
      kWritable,
      kClosing,
    };

    thread_local own< fragment_allocator > ALLOC =
        make< simple_fragment_allocator >();

    using socket_pipe_base =
        pipes::pipe< socket_event, pipes::in< error_code > >;

    template < address_family af, socket_type sock, protocol proto = kNone >
    class coro_socket : public xi::core::coro_io_handler,
                        public channel_interface,
                        public socket_pipe_base {
      own< fragment_allocator > _alloc = make< simple_fragment_allocator >();
      class data_sink;

    public:
      coro_socket()
          : coro_io_handler([this](auto&& yield) { run(yield); })
          , socket_pipe_base(this) {
      }

    private:
      void run(symmetric_coroutine< void >::yield_type& yield) {
        this->push_back(make< data_sink >(yield));
        for (;;) {
          this->read(socket_event::kReadable);
        }
      }
    };

    template < address_family af, socket_type sock, protocol proto >
    class coro_socket< af, sock, proto >::data_sink
        : public pipes::context_aware_filter< own< buffer > > {
      using channel = coro_socket< af, sock, proto >;

      symmetric_coroutine< void >::yield_type& _yield;
      mut<channel> _pipe;

    public:
      data_sink(mut<channel> c, symmetric_coroutine< void >::yield_type& yield)
        : _yield(yield), _pipe(c) {
      }

      void write(mut< context > cx, own< buffer > b) override {
        while (!b->empty()) {
          auto ret = _pipe->write_buffer(edit(b));
          if (XI_UNLIKELY(ret.has_error())) {
            auto error = ret.error();
            if (error == error::kRetry) {
              _pipe->expect_write(true);
              _yield();
            } else {
              close = true;
              if (XI_UNLIKELY(error != error::kEOF)) {
                cx->forward_read(error);
              }
            }
          }
        }
      }
    };

    template < address_family af, socket_type sock, protocol proto = kNone >
    class socket_base : public xi::core::io_handler,
                        public channel_interface,
                        public socket_pipe_base {
      // own< fragment_allocator > _alloc = make< arena_fragment_allocator >(1
      // << 24);
      own< fragment_allocator > _alloc = make< simple_fragment_allocator >();

    public:
      socket_base() : socket_pipe_base(this) {
      }

      virtual ~socket_base() noexcept {
        // std::cout << __PRETTY_FUNCTION__ << std::endl;
      }

      void close() override {
        defer(this, [&] { this->write(socket_event::kClosing); });
      }

    protected:
      mut< fragment_allocator > alloc() override {
        return edit(_alloc);
      }

      void handle_error() override {
        close(); // TODO: handle socket errors
      }

      void handle_close() final override {
        close();
      }

      void handle_read() override {
        this->read(socket_event::kReadable);
      }

      void handle_write() override {
        this->read(socket_event::kWritable);
      }
    };

    template < address_family af, protocol proto = kNone >
    class client_pipe : public socket_base< af, kStream, proto >,
                        public stream_client_socket {
      using endpoint_t = endpoint< af >;

      endpoint_t _remote;

    public:
      struct data_sink;
      class data_source;

      client_pipe(stream_client_socket s);

      using socket_base< af, kStream, proto >::close;
    };

    template < address_family af >
    struct datagram {
      own< buffer > data;
      endpoint< af > remote;
    };

    template < address_family af, protocol proto = kNone >
    class datagram_pipe : public socket_base< af, kDatagram, proto >,
                          public datagram_socket {
      using endpoint_t = endpoint< af >;

    public:
      struct data_sink;
      class data_source;

      datagram_pipe();

      void bind(endpoint_t ep) {
        datagram_socket::bind(ep.to_posix());
      }

      using socket_base< af, kDatagram, proto >::close;
    };

    template < address_family af, protocol proto >
    struct datagram_pipe< af, proto >::data_sink
        : public pipes::filter< socket_event, error_code, datagram< af > > {
      using channel = datagram_pipe< af, proto >;
      using context = typename pipes::
          filter< socket_event, error_code, datagram< af > >::context;
      mut< channel > _pipe;
      data_sink(mut< channel > c) : _pipe(c) {
      }

      void read(mut< context > cx, socket_event e) override {
        switch (e) {
          case socket_event::kReadable: {
            auto b = make< buffer >(_pipe->alloc()->allocate(1 << 16));
            endpoint_t remote;
            auto ret = _pipe->read_buffer_from(edit(b), remote.to_posix());
            // std::cout << "Read " << ret << " bytes from " <<
            // remote.to_string()
            //           << std::endl;
            if (ret.has_error()) {
              if (ret.error() == error::kEOF) {
                _pipe->close();
              } else {
                cx->forward_read(ret.error());
              }
            } else {
              cx->forward_read(datagram< af >{move(b), move(remote)});
            }
          } break;
          case socket_event::kWritable: {
          } break;
          default:
            break;
        };
      }

      void write(mut< context >, socket_event e) override {
        switch (e) {
          case socket_event::kClosing:
            std::cout << __func__ << std::endl;
            _pipe->cancel();
            break;
          default:
            break;
        };
      }

      void write(mut< context >, datagram< af > b) override {
        // std::cout << "Writing " << b->data.size() << " bytes to "
        //           << b->remote.to_string() << std::endl;
        _pipe->write_buffer_to(edit(b.data), b.remote.to_posix());
      }
    };

    template < address_family af, protocol proto >
    datagram_pipe< af, proto >::datagram_pipe() : datagram_socket(af, proto) {
      xi::core::io_handler::descriptor(native_handle());
      this->push_back(make< data_sink >(this));
    }

    template < address_family af, protocol proto >
    struct client_pipe< af, proto >::data_sink
        : public pipes::context_aware_filter< socket_event,
                                              pipes::in< error_code >,
                                              own< buffer > > {
      using channel = client_pipe< af, proto >;
      mut< channel > _pipe;
      buffer _write_buf;
      bool _scheduled_write = false;
      bool _closing         = false;
      i16 _inline_writes    = 0;

      data_sink(mut< channel > c) : _pipe(c) {
      }

      void read_data(bool on_io) {
        auto _b    = make< buffer >();
        bool close = false;
        for (u8 loops = 0; !_closing; ++loops) {
          _b->push_back(_pipe->alloc()->allocate(4 << 12));
          auto ret = _pipe->read_buffer(edit(_b));
          if (ret.has_error()) {
            auto error = ret.error();
            if (error == error::kRetry) {
              _pipe->expect_read(true);
            } else {
              std::cout << "ERROR: " << error.message() << std::endl;
              close = true;
              if (XI_UNLIKELY(error != error::kEOF)) {
                my_context()->forward_read(error);
              }
            }
            break;
          } else {
            if (2 == loops && on_io) {
              defer(_pipe, [this] { read_data(false); });
              break;
            }
          }
        }
        if (!_b->empty()) {
          my_context()->forward_read(move(_b));
        }
        if (close) {
          _pipe->close();
        }
      }

      void write_data() {
        _scheduled_write = false;
        bool close       = false;
        if (!_closing && !_write_buf.empty()) {
          auto ret = _pipe->write_buffer(edit(_write_buf));
          if (XI_UNLIKELY(ret.has_error())) {
            auto error = ret.error();
            if (error == error::kRetry) {
              _pipe->expect_write(true);
            } else {
              close = true;
              if (XI_UNLIKELY(error != error::kEOF)) {
                my_context()->forward_read(error);
              }
            }
          }
        }
        _inline_writes = 0;
        if (close) {
          _pipe->close();
        }
      }

      void read(mut< context >, socket_event e) override {
        switch (e) {
          case socket_event::kReadable: {
            read_data(true);
          } break;
          case socket_event::kWritable: {
            if (!_write_buf.empty()) {
              _pipe->write_buffer(edit(_write_buf));
              _pipe->expect_write(!_write_buf.empty());
              // std::cout << "write queue: " << _write_buf.size() << std::endl;
            }
          } break;
          default:
            break;
        };
      }
      void write(mut< context >, socket_event e) override {
        switch (e) {
          case socket_event::kClosing:
            _closing = true;
            _pipe->cancel();
            break;
          default:
            break;
        };
      }
      void write(mut< context >, own< buffer > b) override {
        if (!_scheduled_write && ++_inline_writes < 1) {
          _pipe->write_buffer(edit(b));
          if (b->empty()) {
            return;
          }
        }
        _write_buf.push_back(move(b));
        // std::cout << "write queue: " << _write_buf.size() << std::endl;
        if (_write_buf.size() > 100 << 20) { // 100 MiB
          std::cout << "Closing connection due to slow reader" << std::endl;
          _pipe->close();
          return;
        }
        if (XI_UNLIKELY(!_closing && !_scheduled_write)) {
          defer(_pipe, [this] { write_data(); });
          _scheduled_write = true;
        }
      }
    };

    template < address_family af, protocol proto >
    client_pipe< af, proto >::client_pipe(stream_client_socket s)
        : stream_client_socket(move(s)) {
      xi::core::io_handler::descriptor(native_handle());
      this->push_back(make< data_sink >(this));
    }

    template < address_family af, protocol proto = kNone >
    struct pipe_factory : public virtual ownership::std_shared {
      using pipe_t     = client_pipe< af, proto >;
      using endpoint_t = endpoint< af >;

      virtual ~pipe_factory()                                 = default;
      virtual own< pipe_t > create_pipe(stream_client_socket) = 0;
    };

    template < address_family af, protocol proto = kNone >
    using pipe_acceptor_base =
        pipes::pipe< pipes::in< error_code >,
                     pipes::in< exception_ptr >,
                     pipes::in< mut< client_pipe< af, proto > > > >;

    template < address_family af, protocol proto = kNone >
    class acceptor_pipe final : public xi::core::io_handler,
                                public stream_server_socket,
                                public pipe_acceptor_base< af, proto >,
                                public virtual ownership::std_shared {
      using endpoint_type    = endpoint< af >;
      using client_pipe_type = client_pipe< af, proto >;

      own< pipe_factory< af, proto > > _pipe_factory;

    public:
      acceptor_pipe()
          : stream_server_socket(af, proto)
          , pipe_acceptor_base< af, proto >(nullptr) {
        xi::core::io_handler::descriptor(native_handle());
      }

      void set_pipe_factory(own< pipe_factory< af, proto > > f) {
        _pipe_factory = move(f);
      }

      void bind(endpoint_type ep) {
        stream_server_socket::bind(ep.to_posix());
      }

    private:
      void handle_write() final override {
      }

      void handle_error() final override {
        close(); // TODO: handle socket errors
      }

      void handle_close() final override {
        close();
      }

      void close() {
        xi::core::io_handler::cancel();
        stream_server_socket::close();
      }

      void handle_read() final override {
        endpoint_type remote;
        auto client = accept();
        if (client.has_error()) {
          this->read(client.error());
        } else {
          this->expect_read(true);
          auto socket = client.move_value();
          try {
            static u16 run_on = 0;
            run_on            = (run_on + 1) % core::bootstrap::cpus();
            shard_at(run_on)->dispatch(
                [ this, s = move(socket), factory = _pipe_factory ]() mutable {
                  own< client_pipe_type > ch;
                  if (is_valid(factory)) {
                    ch = factory->create_pipe(move(s));
                  } else {
                    ch = make< client_pipe_type >(move(s));
                  }
                  auto mut_ch = edit(ch);
                  xi::shard()->reactor()->attach_handler(move(ch));
                  defer(this, [mut_ch, this] { this->read(mut_ch); });
                });
          } catch (...) {
            this->read(current_exception());
          }
        }
      }
    };

    template < address_family af, protocol proto = kNone >
    using pipe_connector_base =
        pipes::pipe< pipes::in< error_code >,
                     pipes::in< exception_ptr >,
                     pipes::in< mut< client_pipe< af, proto > > > >;

    template < address_family af, protocol proto = kNone >
    class connector_pipe final : public pipe_connector_base< af, proto >,
                                 public virtual ownership::std_shared {
      using endpoint_type    = endpoint< af >;
      using client_pipe_type = client_pipe< af, proto >;

      struct connecting_socket : public core::io_handler, socket {
        core::promise< expected< i32 > > _future_descriptor;

        connecting_socket() : socket(af, kStream, proto) {
          xi::core::io_handler::descriptor(native_handle());
        };

        core::event_state expected_state() const noexcept final override {
          return core::kWrite;
        }

        core::future< expected< i32 > > connect(endpoint_type ep) {
          std::cout << "Connecting to " << ep.to_string() << std::endl;
          auto ret = socket::connect(ep.to_posix());
          if (ret.has_error()) {
            auto error = ret.error();
            if (error == io::error::kRetry) {
              std::cout << "Expecting write" << std::endl;
              expect_write(true);
              return _future_descriptor.get_future();
            }
            std::cout << "Connection error: " << ret.error().message()
                      << std::endl;
            XI_SCOPE(exit) {
              close();
            };
            return core::make_ready_future(expected< i32 >(ret.error()));
          }
          auto desc                 = native_handle();
          this->socket::_descriptor = -1;
          io_handler::cancel();
          return core::make_ready_future(expected< i32 >(desc));
        }

      private:
        void close() {
          io_handler::cancel();
          socket::close();
        }

        void handle_write() final override {
          std::cout << __PRETTY_FUNCTION__ << std::endl;
          auto err = get_option< net::option::socket::error >();
          if (err.value() == 0) {
            _future_descriptor.set(native_handle());
            socket::_descriptor = -1;
            io_handler::cancel();
          } else {
            std::cout << "Connection error: " << err.value() << std::endl;
            _future_descriptor.set(error_from_value(err.value()));
            close(); // TODO: handle socket errors
          }
        }

        void handle_read() final override {
          std::cout << __PRETTY_FUNCTION__ << std::endl;
        }

        void handle_error() final override {
          std::cout << __PRETTY_FUNCTION__ << std::endl;
          close(); // TODO: handle socket errors
        }

        void handle_close() final override {
          std::cout << __PRETTY_FUNCTION__ << std::endl;
          close();
        }
      };

      own< pipe_factory< af, proto > > _pipe_factory;

    public:
      connector_pipe() : pipe_connector_base< af, proto >(nullptr) {
      }

      void set_pipe_factory(own< pipe_factory< af, proto > > f) {
        _pipe_factory = move(f);
      }

      core::future< mut< client_pipe_type > > connect(endpoint_type ep) {
        auto promise =
            make_shared< core::promise< mut< client_pipe_type > > >();
        auto s     = make< connecting_socket >();
        auto mut_s = edit(s);
        xi::shard()->reactor()->attach_handler(move(s));
        auto fut = promise->get_future();
        mut_s->connect(ep).then([ this,
                                  p = move(promise) ](expected< i32 > ret) {
          if (XI_UNLIKELY(ret.has_error())) {
            // TODO: Handle error
          } else {
            std::cout << "Ret " << ret << std::endl;
            static u16 run_on = 0;
            run_on            = (run_on + 1) % core::bootstrap::cpus();
            std::cout << "Will run on " << run_on << std::endl;
            shard_at(run_on)->dispatch(
                [ this, s = ret, p = move(p) ]() mutable {
                  own< client_pipe_type > ch;
                  if (is_valid(_pipe_factory)) {
                    ch = _pipe_factory->create_pipe(stream_client_socket(s));
                  } else {
                    ch = make< client_pipe_type >(stream_client_socket(s));
                  }
                  auto mut_ch = edit(ch);
                  xi::shard()->reactor()->attach_handler(move(ch));
                  p->set(mut_ch);
                });
          }
        });
        return fut;
      }
    };

    using ip_datagram        = datagram< kInet >;
    using unix_datagram      = datagram< kUnix >;
    using tcp_stream_pipe    = client_pipe< kInet, kTCP >;
    using udp_datagram_pipe  = datagram_pipe< kInet, kUDP >;
    using unix_datagram_pipe = datagram_pipe< kUnix >;
    using tcp_acceptor_pipe  = acceptor_pipe< kInet, kTCP >;
    using unix_acceptor_pipe = acceptor_pipe< kUnix >;
    using tcp_connector_pipe = connector_pipe< kInet, kTCP >;
  }
}
}
