#pragma once

#include "xi/ext/configure.h"
#include "xi/io/error.h"
#include "xi/io/net/endpoint.h"
#include "xi/util/result.h"

#include <netinet/in.h>
#include <sys/socket.h>

namespace xi {
namespace io {
  namespace net {
    inline namespace posix {

      class socket {
        i32 _descriptor = -1;

      public:
        socket(i32 d);
        socket(i32 address_family, i32 sock_type, i32 protocol);
        ~socket() noexcept;
        XI_CLASS_DEFAULTS(socket, no_copy, move);

        void close();
        i32 native_handle() const;

        result< u32 > read(struct iovec* iov,
                           usize iov_len,
                           opt< posix_endpoint > remote = none) const;

        result< u32 > write(struct iovec* iov,
                            usize iov_len,
                            opt< posix_endpoint > remote = none);

        result< void > bind(posix_endpoint local);
        result< void > listen(posix_endpoint local,
                              opt< u32 > queue_length = none);

        result< socket > accept();

        template < class O >
        result< void > set_option(O option);
        template < class O >
        result< O > get_option();
      };

      inline socket::socket(i32 d) : _descriptor(d) {
        if (_descriptor < 0) {
          throw std::runtime_error("Invalid file descriptor.");
        }
      }

      inline socket::socket(i32 address_family, i32 sock_type, i32 protocol)
          : socket(::socket(address_family, sock_type, protocol)) {
      }

      inline socket::~socket() noexcept {
        close();
      }

      inline void socket::close() {
        ::close(_descriptor);
      }

      inline i32 socket::native_handle() const {
        return _descriptor;
      }

      inline result< u32 > socket::write(struct iovec* iov,
                                         usize iov_len,
                                         opt< posix_endpoint > remote) {
        if (0 == iov_len) {
          return ok(0);
        }
        msghdr msg;
        ::bzero(&msg, sizeof(msg));
        remote.map([&](auto ep) {
          msg.msg_name    = ep.addr;
          msg.msg_namelen = ep.length;
        });
        msg.msg_iov    = iov;
        msg.msg_iovlen = iov_len;
        i32 retval     = sendmsg(_descriptor, &msg, MSG_DONTWAIT);
        if (-1 == retval) {
          if (EAGAIN == errno || EWOULDBLOCK == errno) {
            return {io::error::kRetry};
          }
          if (ECONNRESET == retval || EPIPE == retval) {
            return {io::error::kEOF};
          }
          return result_from_errno< u32 >();
        }
        if (0 == retval) {
          return {io::error::kRetry};
        }
        return ok(retval);
      }

      inline result< u32 > socket::read(struct iovec* iov,
                                        usize iov_len,
                                        opt< posix_endpoint > remote) const {
        if (0 == iov_len) {
          return ok(0);
        }
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov    = iov;
        msg.msg_iovlen = iov_len;
        remote.map([&](auto ep) {
          msg.msg_name    = ep.addr;
          msg.msg_namelen = ep.length;
        });
        i32 retval = recvmsg(_descriptor, &msg, MSG_DONTWAIT);
        if (-1 == retval) {
          if (EAGAIN == errno || EWOULDBLOCK == errno) {
            return {io::error::kRetry};
          }
          if (ECONNRESET == errno) {
            return {io::error::kEOF};
          }
          return result_from_errno< u32 >();
        }
        if (0 == retval) {
          return {io::error::kEOF};
        }
        return ok(retval);
      }

      template < class F, class... A >
      auto posix_to_result(F f, A&&... args) noexcept {
        auto ret       = f(forward< A >(args)...);
        using return_t = result< decltype(ret) >;
        if (-1 == ret) {
          return result_from_errno< decltype(ret) >();
        }
        return return_t{ret};
      }

      inline result< void > socket::bind(posix_endpoint local) {
        i32 ret = ::bind(_descriptor, local.addr, local.length);
        if (-1 == ret) {
          return result_from_errno();
        }
        return ok();
      }

      result< void > socket::listen(posix_endpoint local,
                                    opt< u32 > queue_length) {
        return bind(local).then([&] {
          auto ret = ::listen(native_handle(), queue_length.unwrap_or(1024));
          if (-1 == ret) {
            return result_from_errno();
          }
          return ok();
        });
      }

      inline result< socket > socket::accept() {
        socklen_t socklen = sizeof(sockaddr_in);
        struct sockaddr_in addr;
        i32 retval = ::accept4(native_handle(),
                               (struct sockaddr*)&addr,
                               &socklen,
                               SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (-1 == retval) {
          return result_from_errno< socket >();
        }
        return ok(socket(retval));
      }

      template < class O >
      inline result< void > socket::set_option(O option) {
        auto value = option.value();
        return posix_to_expected(
            setsockopt, _descriptor, O::level, O::name, &value, O::length);
      }

      template < class O >
      inline result< O > socket::get_option() {
        typename O::value_t value;
        u32 len  = O::length;
        auto ret = posix_to_expected(
            getsockopt, _descriptor, O::level, O::name, edit(value), edit(len));
        if (ret.has_error()) {
          return ret.error();
        }
        return {move(value)};
      }
    }
  }
}
}
