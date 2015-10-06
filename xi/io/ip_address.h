#pragma once

#include "xi/ext/configure.h"

#include <netinet/in.h>

namespace xi {
namespace io {

  class ip4_address {
    using bytes = array< uint8_t, 4 >;

  public:
    ip4_address() { _address = 0; }
    ip4_address(ref< bytes > bytes) { *this = bytes; }
    ip4_address(uint32_t long_addr) { *this = long_addr; }

    ip4_address &operator=(ref< bytes > bytes) {
      using namespace std;
      memcpy(&_address, bytes.data(), 4);
      return *this;
    }
    ip4_address &operator=(uint32_t long_addr) {
      _address = ntohl(long_addr);
      return *this;
    }

    static ip4_address any() noexcept { return ip4_address(INADDR_ANY); }

    in_addr_t const &native() const noexcept { return _address; }

  private:
    in_addr_t _address;
  };
}
}
