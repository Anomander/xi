#pragma once
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>

#include "ext/Configure.h"
#include "io/DataMessage.h"

namespace xi {
namespace io {
  namespace test {
    class TcpAcceptorMock {
    public:
      TcpAcceptorMock() : _descriptor(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) {}

      /// Will only be connecting to classes under test,
      /// so localhost only
      int accept(uint8_t port) {
        struct sockaddr_in servaddr;
        ::bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(port);
        int ret = ::bind(_descriptor, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if (-1 == ret) {
          throw system_error(ErrorFromErrno());
        }
        ret = ::listen(_descriptor, 1024); // TODO: Clean up
        if (-1 == ret) {
          throw system_error(ErrorFromErrno());
        }
        ret = ::accept(_descriptor, (struct sockaddr*)nullptr, 0);
        if (-1 == ret) {
          throw system_error(ErrorFromErrno());
        }
        return ret;
      }

    private:
      int _descriptor;
    };

    class TcpSocketMock {
    public:
      TcpSocketMock() : _descriptor(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) {}

      /// Will only be connecting to classes under test,
      /// so localhost only
      void connect(uint16_t port) {
        struct sockaddr_in servaddr;
        ::bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(port);
        auto ret = ::connect(_descriptor, (struct sockaddr*)&servaddr, sizeof(sockaddr_in));
        if (-1 == ret) {
          throw system_error(ErrorFromErrno());
        }
      }

      size_t send(void* data, size_t size) {
        auto ret = ::send(_descriptor, data, size, MSG_NOSIGNAL);
        if (-1 == ret) {
          throw system_error(ErrorFromErrno());
        }
        if (0 == ret) {
          close();
        }
        return ret;
      }

      void sendMessage(void* payload, size_t size) {
        ProtocolHeader hdr;
        hdr.version = 1;
        hdr.type = kData;
        hdr.size = size;
        send(&hdr, sizeof(hdr));

        if (sizeof(payload)) {
          send(payload, size);
        }
      }

      void close() {
        auto ret = ::close(_descriptor);
        if (-1 == ret) {
          throw system_error(ErrorFromErrno());
        }
      }

    private:
      int _descriptor;
    };
  }
}
}
