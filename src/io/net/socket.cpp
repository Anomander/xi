#include "xi/ext/configure.h"
#include "xi/io/net/enumerations.h"
#include "xi/io/net/ip_address.h"
#include "xi/io/net/endpoint.h"
#include "xi/io/error.h"
#include "xi/io/net/socket.h"
#include "xi/io/buffer_iovec_adapter.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <fcntl.h>

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
        : socket(::socket(af, socktype | SOCK_NONBLOCK | SOCK_CLOEXEC,
                          proto)) // throws on failure
    {
    }

    socket::~socket() {
      close();
    }

    stream_client_socket::stream_client_socket(i32 desc,
                                               opt< endpoint< kInet > > remote)
        : socket(desc), _remote(move(remote)) {
    }

    expected< void > socket::close() {
      if (-1 == _descriptor)
        return {};
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

    void server_socket::bind(ref< endpoint< kInet > > local) {
      struct sockaddr_in servaddr;
      ::bzero(&servaddr, sizeof(servaddr));
      servaddr.sin_family = AF_INET;
      servaddr.sin_addr.s_addr = local.address.native();
      servaddr.sin_port = htons(local.port);
      i32 ret =
          ::bind(_descriptor, (struct sockaddr *)&servaddr, sizeof(servaddr));
      if (-1 == ret) {
        throw_errno();
      }
      ret = ::listen(_descriptor, 1024); // TODO: clean up
      if (-1 == ret) {
        throw_errno();
      }
    }

    expected< stream_client_socket > stream_server_socket::accept() {
      socklen_t socklen = sizeof(sockaddr_in);
      struct sockaddr_in addr;
      i32 retval = ::accept4(_descriptor, (struct sockaddr *)&addr, &socklen,
                             SOCK_NONBLOCK | SOCK_CLOEXEC);
      if (-1 == retval) {
        return error_from_errno();
      }
      endpoint< kInet > remote;
      remote.address = addr.sin_addr.s_addr;
      remote.port = ntohs(addr.sin_port);
      return stream_client_socket(retval, some(remote));
    }

    void stream_server_socket::set_child_options(initializer_list<option_t> options) {
      _child_options.insert (end(_child_options), options);
    }

    expected< i32 > socket::write_iov(
        struct iovec *iov, usize iov_len,
        opt< ref< endpoint< kInet > > > remote) const {
      if (0 == iov_len) {
        return 0;
      }
      socklen_t socklen = sizeof(sockaddr_in);
      struct sockaddr_in addr;
      msghdr msg;
      memset(&msg, 0, sizeof(msg));
      msg.msg_iov = iov;
      msg.msg_iovlen = iov_len;
      msg.msg_name = &addr;
      msg.msg_namelen = socklen;
      i32 retval = sendmsg(_descriptor, &msg, MSG_DONTWAIT);
      if (-1 == retval) {
        return error_from_errno();
      }
      if (0 == retval) {
        return error_code{io::error::kUnableToReadMessageHeader};
      }
      return retval;
    }
    expected< i32 > stream_client_socket::write(
        mut< buffer > b, opt< ref< endpoint< kInet > > > remote) const {
      if (b->size() == 0) {
        return 0;
      }
      iovec iov[IOV_MAX];
      i32 ret = 0;
      u64 len = 0;
      do {
        len = buffer::iovec_adapter::fill_readable_iovec(*(b), iov, IOV_MAX);
        auto r = write_iov(iov, len, move(remote));
        if (r.has_error()) {
          return r;
        }
        b->skip_bytes(r);
        ret += r;
      } while (IOV_MAX == len);
      return ret;
    }
    expected< i32 > socket::read_iov(
        struct iovec *iov, usize iov_len,
        opt< mut< endpoint< kInet > > > remote) const {
      if (0 == iov_len) {
        return 0;
      }
      socklen_t socklen = sizeof(sockaddr_in);
      struct sockaddr_in addr;
      msghdr msg;
      memset(&msg, 0, sizeof(msg));
      msg.msg_iov = iov;
      msg.msg_iovlen = iov_len;
      msg.msg_name = &addr;
      msg.msg_namelen = socklen;
      i32 retval = recvmsg(_descriptor, &msg, MSG_DONTWAIT);
      if (-1 == retval) {
        return error_from_errno();
      }
      if (0 == retval) {
        return error_code{io::error::kEOF};
      }
      if (remote.is_some()) {
        remote.unwrap()->address = addr.sin_addr.s_addr;
        remote.unwrap()->port = ntohs(addr.sin_port);
      }
      return retval;
    }

    expected< i32 > stream_client_socket::read(
        mut< buffer > b, opt< mut< endpoint< kInet > > > remote) const {
      iovec iov[1];
      if (0 == b->tailroom())
        return 0;
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

    expected< usize > socket::readable_bytes() {
      usize sz = 0;
      i32 retval = ::ioctl(_descriptor, FIONREAD, &sz);
      if (-1 == retval) {
        return error_from_errno();
      }
      return sz;
    }
  }
}
}
