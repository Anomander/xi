#include "xi/ext/configure.h"
#include "xi/io/net/socket.h"
#include "xi/io/buffer_iovec_adapter.h"
#include "xi/io/error.h"
#include "xi/io/net/endpoint.h"
#include "xi/io/net/enumerations.h"
#include "xi/io/net/ip_address.h"

#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

namespace xi {
namespace io {
  namespace net {
    namespace {
      [[gnu::noreturn]] void throw_errno() {
        throw system_error(error_from_errno());
      }
    }

    socket::socket(socket &&other) : _descriptor(other._descriptor) {
      other._descriptor = -1;
    }

    socket::socket(i32 af, i32 socktype, i32 proto)
        : socket(::socket(af,
                          socktype | SOCK_NONBLOCK | SOCK_CLOEXEC,
                          proto)) // throws on failure
    {
    }

    socket::~socket() {
      close();
    }

    stream_client_socket::stream_client_socket(i32 desc) : data_socket(desc) {
    }

    expected< void > socket::close() {
      if (-1 == _descriptor) {
        return {};
      }
      std::cout << "Closing socket " << _descriptor << std::endl;
      auto ret = ::close(_descriptor);
      if (-1 == ret) {
        return error_from_errno();
      }
      return {};
    }

    expected< socket > socket::create(int af, i32 socktype, i32 proto) {
      auto ret = ::socket(af, socktype, proto);
      if (-1 == ret) {
        return error_from_errno();
      }
      return socket{ret};
    }

    void bindable_socket_adapter::bind(posix_endpoint local) {
      i32 ret = ::bind(_descriptor, local.addr, local.length);
      if (-1 == ret) {
        throw_errno();
      }
    }

    void stream_server_socket::bind(posix_endpoint local) {
      bindable_socket_adapter::bind(local);
      auto ret = ::listen(native_handle(), 1024); // TODO: clean up
      if (-1 == ret) {
        throw_errno();
      }
    }

    expected< stream_client_socket > stream_server_socket::accept() {
      socklen_t socklen = sizeof(sockaddr_in);
      struct sockaddr_in addr;
      i32 retval = ::accept4(native_handle(),
                             (struct sockaddr *)&addr,
                             &socklen,
                             SOCK_NONBLOCK | SOCK_CLOEXEC);
      if (-1 == retval) {
        return error_from_errno();
      }
      return stream_client_socket(retval);
    }

    void stream_server_socket::set_child_options_list(
        initializer_list< option_t > options) {
      _child_options.insert(end(_child_options), options);
    }

    expected< i32 > data_socket::write_iov(struct iovec *iov,
                                           usize iov_len,
                                           opt< posix_endpoint > remote) const {
      if (0 == iov_len) {
        return 0;
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
        return error_from_errno();
      }
      if (0 == retval) {
        return error_code{io::error::kUnableToReadMessageHeader};
      }
      return retval;
    }
    expected< i32 > data_socket::write(mut< buffer > b,
                                       opt< posix_endpoint > remote) const {
      if (b->size() == 0) {
        return 0;
      }
      iovec iov[IOV_MAX];
      i32 ret = 0;
      u64 len = 0;
      do {
        len    = buffer::iovec_adapter::fill_readable_iovec(*(b), iov, IOV_MAX);
        auto r = write_iov(iov, len, move(remote));
        if (r.has_error()) {
          return r;
        }
        b->skip_bytes(r);
        ret += r;
      } while (IOV_MAX == len);
      return ret;
    }

    expected< i32 > stream_client_socket::write_buffer(mut< buffer > b) const {
      return data_socket::write(b, none);
    }

    expected< i32 > datagram_socket::write_buffer_to(
        mut< buffer > b, posix_endpoint remote) const {
      return data_socket::write(b, some(remote));
    }

    expected< i32 > data_socket::read_iov(struct iovec *iov,
                                          usize iov_len,
                                          opt< posix_endpoint > remote) const {
      if (0 == iov_len) {
        return 0;
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
        return error_from_errno();
      }
      if (0 == retval) {
        return error_code{io::error::kEOF};
      }
      return retval;
    }

    expected< i32 > data_socket::read(mut< buffer > b,
                                      opt< posix_endpoint > remote) const {
      iovec iov[1];
      if (0 == b->tailroom()) {
        return 0;
      }
      buffer::iovec_adapter::fill_writable_iovec(b, iov);
      auto r = read_iov(iov, 1, move(remote));
      if (!r.has_error()) {
        buffer::iovec_adapter::report_written(b, r);
      } else {
        if (r.error() == error::kEOF) {
          return error_code{error::kEOF};
        }
      }
      return r;
    }

    expected< i32 > stream_client_socket::read_buffer(mut< buffer > b) const {
      return data_socket::read(b, none);
    }

    expected< i32 > datagram_socket::read_buffer_from(
        mut< buffer > b, posix_endpoint remote) const {
      return data_socket::read(b, some(remote));
    }

    expected< usize > socket::readable_bytes() {
      usize sz   = 0;
      i32 retval = ::ioctl(_descriptor, FIONREAD, &sz);
      if (-1 == retval) {
        return error_from_errno();
      }
      return sz;
    }
  }
}
}
