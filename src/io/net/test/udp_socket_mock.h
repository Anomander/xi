#pragma once
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>

#include "xi/ext/configure.h"

namespace xi {
namespace test {
  namespace io {
    class udp_socket_mock {
    public:
      udp_socket_mock()
          : _descriptor(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {
      }

      /// will only be connecting to classes under test,
      /// so localhost only
      void connect(uint16_t port) {
        struct sockaddr_in servaddr;
        ::bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(port);
        auto ret = ::connect(_descriptor, (struct sockaddr *)&servaddr,
                             sizeof(sockaddr_in));
        if (-1 == ret) {
          throw system_error(error_from_errno());
        }
      }

      size_t send(void *data, size_t size) {
        if (0 == size) {
          return 0;
        }
        auto ret = ::send(_descriptor, data, size, MSG_NOSIGNAL);
        if (-1 == ret) {
          throw system_error(error_from_errno());
        }
        if (0 == ret) {
          close();
        }
        return ret;
      }

      size_t recv(void *data, size_t size) {
        if (0 == size) {
          return 0;
        }
        auto ret = ::recv(_descriptor, data, size, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (-1 == ret) {
          throw system_error(error_from_errno());
        }
        if (0 == ret) {
          close();
        }
        return ret;
      }

      /// will only be connecting to classes under test,
      /// so localhost only
      void bind(uint16_t port) {
        struct sockaddr_in servaddr;
        ::bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(port);
        auto one = 1;
        if (setsockopt(_descriptor, SOL_SOCKET, SO_REUSEADDR, &one,
                       sizeof(one)) < 0)
          throw system_error(error_from_errno());
        int ret =
            ::bind(_descriptor, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if (-1 == ret) {
          throw system_error(error_from_errno());
        }
      }

      void close() {
        if (-1 == _descriptor)
          return;
        auto ret = ::close(_descriptor);
        if (-1 == ret) {
          throw system_error(error_from_errno());
        }
        _descriptor = -1;
      }

    private:
      int _descriptor;
    };
  }
}
}
