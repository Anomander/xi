#pragma once

#include "xi/ext/configure.h"
#include "xi/core/event_handler.h"
#include "xi/io/channel_interface.h" // TODO: ??
#include "xi/io/net/enumerations.h"
#include "xi/io/net/socket/posix.hpp"
#include "xi/io/pipes/all.h"

namespace xi {
namespace io {
  namespace net {
    enum class socket_event {
      kReadable,
      kWritable,
      kClosing,
    };

    template < address_family af, socket_type sock, protocol proto = kNone >
    class socket_base : public core::io_handler,
                        public channel_interface { // TODO: ??
      using pipe_t  = io::pipes::pipe< socket_event, pipes::in< error_code > >;
      using alloc_t = simple_fragment_allocator;

      own< alloc_t > _alloc = make< alloc_t >();
      pipe_t _pipe;
      socket _socket;

    public:
      socket_base(socket s) : _pipe(this), _socket(move(s)) {
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

    template < address_family AF, protocol Proto = kNone >
    class client_channel : public socket_base< AF, kStream, Proto > {
      using super_t    = socket_base< AF, kStream, Proto >;
      using endpoint_t = endpoint< AF >;

      endpoint_t _remote;

    public:
      struct data_sink;

      client_channel(socket s);

      using super_t::close;
    };
  }
}
}
