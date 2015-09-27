#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {

  class ip4Address {
    using bytes = array< uint8_t, 4 >;

  public:
    ip4Address() { _address = 0; }
    ip4Address(ref< bytes > bytes) { *this = bytes; }
    ip4Address(uint32_t long_addr) { *this = long_addr; }

    ip4Address &operator=(ref< bytes > bytes) {
      using namespace std;
      memcpy(&_address, bytes.data(), 4);
      return *this;
    }
    ip4Address &operator=(uint32_t long_addr) {
      _address = ntohl(long_addr);
      return *this;
    }

    static ip4Address any() noexcept { return ip4Address(INADDR_ANY); }

    in_addr_t const &native() const noexcept { return _address; }

  private:
    in_addr_t _address;
  };
}
}
