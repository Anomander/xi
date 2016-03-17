#pragma once

#include "xi/ext/configure.h"
#include "xi/io/net/enumerations.h"
#include "xi/io/net/ip_address.h"

namespace xi {
namespace io {
  namespace net {

  template < address_family af > class endpoint;

  template <> class endpoint< kInet > {
  public:
    ip4_address address;
    u16 port;

    endpoint() : endpoint(-1) {}
    endpoint(u16 p) : address(ip4_address::any()), port(p) {}
  };
  }
}
}
