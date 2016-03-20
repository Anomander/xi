#pragma once

#include "xi/ext/configure.h"
#include "xi/io/net/enumerations.h"
#include "xi/io/net/ip_address.h"

#include <sys/un.h>

namespace xi {
namespace io {
  namespace net {

    template < address_family af >
    class endpoint;

    struct posix_endpoint {
      sockaddr* addr;
      socklen_t length;
    };

    template <>
    class endpoint< kInet > {
      sockaddr_in _addr;

    public:
      endpoint() : endpoint(-1) {
      }

      endpoint(u16 p) {
        ::bzero(&_addr, sizeof(sockaddr_in));
        _addr.sin_family = AF_INET;
        _addr.sin_addr.s_addr = INADDR_ANY;
        _addr.sin_port = htons(p);
      }

      u16 port() const {
        return ntohs(_addr.sin_port);
      }

      ip4_address address() const {
        return {_addr.sin_addr.s_addr};
      }

      posix_endpoint to_posix() const {
        return {(struct sockaddr*)&_addr, sizeof(sockaddr_in)};
      }

      string to_string() const {
        return address().to_string() + ':' + std::to_string(port());
      }
    };

    template <>
    class endpoint< kUnix > {
      sockaddr_un _addr;

    public:
      endpoint() = default;

      template < usize N >
      endpoint(char const(&path)[N]) {
        static_assert(N <= sizeof(sockaddr_un::sun_path), "");
        ::bzero(&_addr, sizeof(sockaddr_in));
        _addr.sun_family = AF_UNIX;
        ::memcpy(_addr.sun_path, path, N);
      }

      endpoint(char const* path, usize len = -1) {
        if ((usize)-1 == len) {
          len = strlen(path);
        }
        assert(len <= sizeof(sockaddr_un::sun_path));
        ::bzero(&_addr, sizeof(sockaddr_in));
        _addr.sun_family = AF_UNIX;
        ::memcpy(_addr.sun_path, path, len);
      }

      char const* path() const {
        return _addr.sun_path;
      }

      posix_endpoint to_posix() const {
        return {(struct sockaddr*)&_addr, sizeof(sockaddr_un)};
      }

      string to_string() const {
        return path();
      }
    };
  }
}
}
