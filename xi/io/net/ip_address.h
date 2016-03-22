#pragma once

#include "xi/ext/configure.h"

#include <arpa/inet.h>
#include <netinet/in.h>

namespace xi {
namespace io {
  namespace net {

    class ip4_address {
      using bytes = array< u8, 4 >;

    public:
      ip4_address() {
        _address = 0;
      }
      ip4_address(ref< bytes > bytes) {
        *this = bytes;
      }
      ip4_address(u32 long_addr) {
        *this = long_addr;
      }

      ip4_address &operator=(ref< bytes > bytes) {
        using namespace std;
        memcpy(&_address, bytes.data(), 4);
        return *this;
      }
      ip4_address &operator=(ref< in_addr_t > addr) {
        _address = addr;
        return *this;
      }
      static ip4_address any() noexcept {
        return ip4_address(INADDR_ANY);
      }

      in_addr_t const &native() const noexcept {
        return _address;
      }

      string to_string() const {
        return inet_ntoa(in_addr{_address});
      }

    private:
      in_addr_t _address;
    };
  }
}
}
