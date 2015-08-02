#pragma once

#include "ext/Configure.h"
#include "io/Enumerations.h"
#include "io/IpAddress.h"

namespace xi {
namespace io {

  template < AddressFamily af >
  class Endpoint;

  template <>
  class Endpoint< kInet > {
  public:
    Ip4Address address;
    uint16_t port;

    Endpoint() : Endpoint(-1) {
    }
    Endpoint(uint16_t p) : address(Ip4Address::any()), port(p) {
    }
  };
}
}
