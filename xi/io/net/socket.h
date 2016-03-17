#pragma once

#include "xi/ext/configure.h"
#include "xi/io/net/enumerations.h"
#include "xi/io/net/ip_address.h"
#include "xi/io/net/endpoint.h"
#include "xi/io/error.h"
#include "xi/io/buffer.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <fcntl.h>

namespace xi {
namespace io {
  namespace net {

    class socket {
    protected:
      i32 _descriptor = -1;

    protected:
      socket(i32 descriptor) : _descriptor(descriptor) {
        if (-1 == _descriptor) {
          throw system_error(error_from_errno());
        }
      }

    public:
      XI_CLASS_DEFAULTS(socket, no_copy);

      socket(socket &&);
      socket(i32 af, i32 socktype, i32 proto);
      virtual ~socket();

      template < class O > expected< void > set_option(O option);
      template < class O > expected< O > get_option();

      static expected< socket > create(int af, i32 socktype, i32 proto);

      expected< void > close();
      expected< usize > readable_bytes();

      i32 native_handle() const;

    protected:
      expected< i32 > write_iov(
          struct iovec *iov, usize iov_len,
          opt< ref< endpoint< kInet > > > remote = none) const;

      expected< i32 > read_iov(
          struct iovec *iov, usize iov_len,
          opt< mut< endpoint< kInet > > > remote = none) const;
    };

    inline i32 socket::native_handle() const {
      return _descriptor;
    }

    struct server_socket : public socket {
      using socket::socket;
      void bind(ref< endpoint< kInet > > local);
    };

    struct stream_client_socket : public socket {
      opt< endpoint< kInet > > _remote = none;

    public:
      stream_client_socket(i32 desc, opt< endpoint< kInet > > remote = none);

      expected< i32 > write(
          mut< buffer > b, opt< ref< endpoint< kInet > > > remote = none) const;

      expected< i32 > read(mut< buffer > b,
                           opt< mut< endpoint< kInet > > > remote = none) const;
    };

    class stream_server_socket : public server_socket {
      using option_t = tuple< int, int, int, int >;
      vector< option_t > _child_options;

    public:
      using server_socket::server_socket;
      expected< stream_client_socket > accept();

      template < class O > void set_child_option(O option) {
        _child_options.push_back(
            make_tuple(O::level, O::name, option.value(), O::length));
      }
      template < class... O > void set_child_options(O... options) {
        this->set_child_options(make_tuple(options)...);
      }
      void set_child_options(initializer_list< option_t >);
    };

    template < class O > inline expected< void > socket::set_option(O option) {
      auto value = option.value();
      return posix_to_expected(setsockopt, _descriptor, O::level, O::name,
                               &value, O::length);
    }

    template < class O > inline expected< O > socket::get_option() {
      typename O::value_t value;
      auto ret = posix_to_expected(getsockopt, _descriptor, O::level, O::name,
                                   &value, O::length);
      if (ret.has_error()) {
        return ret;
      }
      return {move(value)};
    }
  }
}
}
