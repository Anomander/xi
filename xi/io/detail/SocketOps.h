#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "xi/io/Enumerations.h"
#include "xi/io/Endpoint.h"
#include "xi/io/Error.h"
#include "xi/io/IpAddress.h"
#include "xi/io/DataMessage.h"
#include "xi/io/StreamBuffer.h"

#include "xi/ext/Expected.h"

namespace xi {
namespace io {
  namespace detail {
    namespace socket {

      template < class Option >
      inline Expected< void > setOption(int descriptor, Option option) {
        auto value = option.value();
        auto ret = ::setsockopt(descriptor, option.level(), option.name(), &value, option.length());
        if (-1 == ret) {
          return ErrorFromErrno();
        }
        return {};
      }

      inline Expected< void > close(int descriptor) {
        auto ret = ::close(descriptor);
        if (-1 == ret) {
          return ErrorFromErrno();
        }
        return {};
      }

      inline Expected< int > create(int af, int socktype, int proto) {
        auto ret = ::socket(af, socktype, proto);
        if (-1 == ret) {
          return ErrorFromErrno();
        }
        return ret;
      }

      inline Expected< void > bind(int descriptor, ref< Endpoint< kInet > > local) {
        struct sockaddr_in servaddr;
        ::bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = local.address.native();
        servaddr.sin_port = htons(local.port);
        int ret = ::bind(descriptor, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if (-1 == ret) {
          return ErrorFromErrno();
        }
        ret = ::listen(descriptor, 1024); // TODO: Clean up
        if (-1 == ret) {
          return ErrorFromErrno();
        }
        return {};
      }

      inline Expected< int > accept(int descriptor, mut< Endpoint< kInet > > remote) {
        socklen_t socklen = sizeof(sockaddr_in);
        struct sockaddr_in addr;
        auto retval = ::accept(descriptor, (struct sockaddr *)&addr, &socklen);
        if (-1 == retval) {
          return ErrorFromErrno();
        }
        remote->address = addr.sin_addr.s_addr;
        remote->port = ntohs(addr.sin_port);
        return retval;
      }

      inline Expected< int > write(int descriptor, void *bytes, size_t sz) {
        int retval = send(descriptor, bytes, sz, MSG_DONTWAIT | MSG_NOSIGNAL);
        if (-1 == retval) {
          return ErrorFromErrno();
        }
        return retval;
      }

      inline Expected< int > write(int descriptor, ByteRange range) {
        if (range.empty()) {
          return 0;
        }
        return write(descriptor, range.data, range.size);
      }

      inline Expected< int > write(int descriptor, ref< StreamBuffer > range) {
        ByteRange ranges[256];
        auto rangeCount = range.fillVec(ranges, 256);
        iovec iov[rangeCount];
        int i = 0;
        for (auto &range : ranges) {
          if (range.empty()) {
            continue;
          }
          iov[i].iov_base = range.data;
          iov[i++].iov_len = range.size;
        }
        msghdr msg;
        msg.msg_iov = iov;
        msg.msg_iovlen = rangeCount;
        msg.msg_name = nullptr;
        msg.msg_namelen = 0;
        return sendmsg(descriptor, &msg, MSG_DONTWAIT);
      }

      inline Expected< int > read(int descriptor, void *bytes, size_t sz) {
        int retval = recv(descriptor, bytes, sz, MSG_DONTWAIT);
        if (-1 == retval) {
          return ErrorFromErrno();
        }
        if (0 == retval) {
          return error_code{io::error::kEof};
        }
        return retval;
      }

      inline Expected< int > readv(int descriptor, initializer_list< ByteRange > ranges,
                                   opt< mut< Endpoint< kInet > > > remote = none) {
        socklen_t socklen = sizeof(sockaddr_in);
        struct sockaddr_in addr;
        iovec iov[ranges.size()];
        int i = 0;
        for (auto &range : ranges) {
          if (range.empty()) {
            continue;
          }
          iov[i].iov_base = range.data;
          iov[i++].iov_len = range.size;
        }
        msghdr msg;
        msg.msg_iov = iov;
        msg.msg_iovlen = ranges.size();
        msg.msg_name = &addr;
        msg.msg_namelen = socklen;
        int retval = recvmsg(descriptor, &msg, MSG_DONTWAIT);
        if (-1 == retval) {
          return ErrorFromErrno();
        }
        if (0 == retval) {
          return error_code{io::error::kEof};
        }
        if (remote) {
          remote.get()->address = addr.sin_addr.s_addr;
          remote.get()->port = ntohs(addr.sin_port);
        }
        return retval;
      }

      inline Expected< int > peek(int descriptor, void *bytes, size_t sz) {
        int retval = recv(descriptor, bytes, sz, MSG_DONTWAIT | MSG_PEEK);
        if (-1 == retval) {
          return ErrorFromErrno();
        }
        if (0 == retval) {
          return error_code{io::error::kEof};
        }
        return retval;
      }

      inline Expected< size_t > readableBytes(int descriptor) {
        size_t sz = 0;
        int retval = ::ioctl(descriptor, FIONREAD, &sz);
        if (-1 == retval) {
          return ErrorFromErrno();
        }
        return sz;
      }
    }
  }
}
}
