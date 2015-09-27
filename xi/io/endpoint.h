#pragma once

#include "xi/ext/configure.h"
#include "xi/io/enumerations.h"
#include "xi/io/ip_address.h"

namespace xi {
namespace io {

  template < address_family af > class endpoint;

  template <> class endpoint< kInet > {
  public:
    ip4Address address;
    uint16_t port;

    endpoint() : endpoint(-1) {}
    endpoint(uint16_t p) : address(ip4Address::any()), port(p) {}
  };
}
}
