#pragma once
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>

#include "xi/ext/configure.h"
#include "xi/io/data_message.h"

namespace xi {
namespace io {
  namespace test {
    class tcp_acceptor_mock {
    public:
      tcp_acceptor_mock()
          : _descriptor(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) {}

      /// will only be connecting to classes under test,
      /// so localhost only
      int accept(uint8_t port) {
        struct sockaddr_in servaddr;
        ::bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(port);
        int ret =
            ::bind(_descriptor, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if (-1 == ret) { throw system_error(error_from_errno()); }
        ret = ::listen(_descriptor, 1024); // TODO: clean up
        if (-1 == ret) { throw system_error(error_from_errno()); }
        ret = ::accept(_descriptor, (struct sockaddr *)nullptr, 0);
        if (-1 == ret) { throw system_error(error_from_errno()); }
        return ret;
      }

    private:
      int _descriptor;
    };

    class tcp_socket_mock {
    public:
      tcp_socket_mock()
          : _descriptor(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) {}

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
        if (-1 == ret) { throw system_error(error_from_errno()); }
      }

      size_t send(void *data, size_t size) {
        auto ret = ::send(_descriptor, data, size, MSG_NOSIGNAL);
        if (-1 == ret) { throw system_error(error_from_errno()); }
        if (0 == ret) { close(); }
        return ret;
      }

      void send_message(void *payload, size_t size) {
        protocol_header hdr;
        hdr.version = 1;
        hdr.type = kData;
        hdr.size = size;
        send(&hdr, sizeof(hdr));

        if (sizeof(payload)) { send(payload, size); }
      }

      void close() {
        auto ret = ::close(_descriptor);
        if (-1 == ret) { throw system_error(error_from_errno()); }
      }

    private:
      int _descriptor;
    };
  }
}
}
