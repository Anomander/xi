#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace io {

  class Ip4Address {
    using Bytes = array< uint8_t, 4 >;

  public:
    Ip4Address() { _address = 0; }
    Ip4Address(ref< Bytes > bytes) { *this = bytes; }
    Ip4Address(uint32_t longAddr) { *this = longAddr; }

    Ip4Address& operator=(ref< Bytes > bytes) {
      using namespace std;
      memcpy(&_address, bytes.data(), 4);
      return *this;
    }
    Ip4Address& operator=(uint32_t longAddr) {
      _address = ntohl(longAddr);
      return *this;
    }

    static Ip4Address any() noexcept { return Ip4Address(INADDR_ANY); }

    in_addr_t const& native() const noexcept { return _address; }

  private:
    in_addr_t _address;
  };
}
}
