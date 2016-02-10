#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "xi/io/enumerations.h"
#include "xi/io/endpoint.h"
#include "xi/io/error.h"
#include "xi/io/ip_address.h"
#include "xi/io/data_message.h"
#include "xi/io/stream_buffer.h"
#include "xi/io/buf/buffer.h"
#include "xi/io/buf/chain.h"
#include "xi/io/buf/chain_iovec_adapter.h"

#include "xi/ext/expected.h"

namespace xi {
namespace io {
  namespace detail {
    namespace socket {

      template < class O >
      inline expected< void > set_option(int descriptor, O option) {
        auto value = option.value();
        auto ret = ::setsockopt(descriptor, option.level(), option.name(),
                                &value, option.length());
        if (-1 == ret) { return error_from_errno(); }
        return {};
      }

      inline expected< void > close(int descriptor) {
        auto ret = ::close(descriptor);
        if (-1 == ret) { return error_from_errno(); }
        return {};
      }

      inline expected< int > create(int af, int socktype, int proto) {
        auto ret = ::socket(af, socktype, proto);
        if (-1 == ret) { return error_from_errno(); }
        return ret;
      }

      inline expected< void > bind(int descriptor,
                                   ref< endpoint< kInet > > local) {
        struct sockaddr_in servaddr;
        ::bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = local.address.native();
        servaddr.sin_port = htons(local.port);
        int ret =
            ::bind(descriptor, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if (-1 == ret) { return error_from_errno(); }
        ret = ::listen(descriptor, 1024); // TODO: clean up
        if (-1 == ret) { return error_from_errno(); }
        return {};
      }

      inline expected< int > accept(int descriptor,
                                    mut< endpoint< kInet > > remote) {
        socklen_t socklen = sizeof(sockaddr_in);
        struct sockaddr_in addr;
        auto retval = ::accept(descriptor, (struct sockaddr *)&addr, &socklen);
        if (-1 == retval) { return error_from_errno(); }
        remote->address = addr.sin_addr.s_addr;
        remote->port = ntohs(addr.sin_port);
        return retval;
      }

      inline expected< int > write(int descriptor, void *bytes, size_t sz) {
        int retval = send(descriptor, bytes, sz, MSG_DONTWAIT | MSG_NOSIGNAL);
        if (-1 == retval) { return error_from_errno(); }
        return retval;
      }

      inline expected< int > write(int descriptor, byte_range range) {
        if (range.empty()) { return 0; }
        return write(descriptor, range.data, range.size);
      }
      inline expected< int > write(
          int descriptor, struct iovec *iov, size_t iov_len,
          opt< ref< endpoint< kInet > > > remote = none) {
        msghdr msg;
        socklen_t socklen = sizeof(sockaddr_in);
        struct sockaddr_in addr;
        msg.msg_iov = iov;
        msg.msg_iovlen = iov_len;
        msg.msg_name = &addr;
        msg.msg_namelen = socklen;
        int retval = sendmsg(descriptor, &msg, MSG_DONTWAIT);
        if (-1 == retval) { return error_from_errno(); }
        if (0 == retval) { return error_code{io::error::kEOF}; }
        return retval;
      }

      inline expected< int > write(int descriptor, ref< stream_buffer > range) {
        byte_range ranges[256];
        auto range_count = range.fill_vec(ranges, 256);
        iovec iov[range_count];
        int i = 0;
        for (auto &range : ranges) {
          if (range.empty()) { continue; }
          iov[i].iov_base = range.data;
          iov[i++].iov_len = range.size;
        }
        return write(descriptor, iov, range_count);
      }

      inline expected< int > write(
          int descriptor, mut< buffer::chain > ch,
          opt< ref< endpoint< kInet > > > remote = none) {
        if (ch->size() == 0) { return 0; }
        iovec iov[64];
        auto len =
            buffer::chain::iovec_adapter::fill_readable_iovec(*ch, iov, 64);
        std::cout << "Sending " << len << " buffers to " << descriptor << "\n";
        for (unsigned i = 0; i < len; ++i) {
          std::cout << i << ": " << iov[i].iov_base << " : " << iov[i].iov_len
                    << "\n";
          for (unsigned j = 0; j < iov[i].iov_len; ++j) {
            std::cout << (int)((uint8_t *)iov[i].iov_base)[j] << " ";
          }
          std::cout << "\n";
        }
        std::cout << std::endl;
        auto ret = write(descriptor, iov, len, move(remote));
        if (!ret.has_error()) {
          std::cout << "Wrote " << ret << std::endl;
          ch->consume_head(ret);
        } else {
          std::cout << "Write error: " << ret.error().message() << std::endl;
        }
        return ret;
      }

      inline expected< int > read(int descriptor, void *bytes, size_t sz) {
        int retval = recv(descriptor, bytes, sz, MSG_DONTWAIT);
        if (-1 == retval) { return error_from_errno(); }
        if (0 == retval) { return error_code{io::error::kEOF}; }
        return retval;
      }

      inline expected< int > read(
          int descriptor, struct iovec *iov, size_t iov_len,
          opt< mut< endpoint< kInet > > > remote = none) {
        socklen_t socklen = sizeof(sockaddr_in);
        struct sockaddr_in addr;
        msghdr msg;
        msg.msg_iov = iov;
        msg.msg_iovlen = iov_len;
        msg.msg_name = &addr;
        msg.msg_namelen = socklen;
        int retval = recvmsg(descriptor, &msg, MSG_DONTWAIT);
        if (-1 == retval) { return error_from_errno(); }
        if (0 == retval) { return error_code{io::error::kEOF}; }
        if (remote.is_some()) {
          remote.unwrap()->address = addr.sin_addr.s_addr;
          remote.unwrap()->port = ntohs(addr.sin_port);
        }
        std::cout << "Read " << retval << " from " << descriptor << std::endl;
        return retval;
      }

      inline expected< int > read(
          int descriptor, initializer_list< byte_range > ranges,
          opt< mut< endpoint< kInet > > > remote = none) {
        iovec iov[ranges.size()];
        int i = 0;
        for (auto &range : ranges) {
          if (range.empty()) { continue; }
          iov[i].iov_base = range.data;
          iov[i++].iov_len = range.size;
        }
        return read(descriptor, iov, ranges.size(), move(remote));
      }

      inline expected< int > read(
          int descriptor, mut< buffer::chain > ch,
          opt< mut< endpoint< kInet > > > remote = none) {
        iovec iov[64];
        auto len =
            buffer::chain::iovec_adapter::fill_writable_iovec(ch, iov, 64);
        auto ret = read(descriptor, iov, len, move(remote));
        if (!ret.has_error()) { ch->append_tail(ret); }
        return ret;
      }

      inline expected< int > peek(int descriptor, void *bytes, size_t sz) {
        int retval = recv(descriptor, bytes, sz, MSG_DONTWAIT | MSG_PEEK);
        if (-1 == retval) { return error_from_errno(); }
        if (0 == retval) { return error_code{io::error::kEOF}; }
        return retval;
      }

      inline expected< size_t > readable_bytes(int descriptor) {
        size_t sz = 0;
        int retval = ::ioctl(descriptor, FIONREAD, &sz);
        if (-1 == retval) { return error_from_errno(); }
        return sz;
      }
    }
  }
}
}
