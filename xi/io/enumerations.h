#pragma once

#include <netinet/in.h>

namespace xi {
namespace io {

  enum address_family {
    kInet = AF_INET,
    kInet6 = AF_INET6,
    kUnix = AF_UNIX,
  };

  enum socket_type {
    kDatagram = SOCK_DGRAM,
    kStream = SOCK_STREAM,
  };

  enum protocol {
    kNone = 0,
    kTCP = IPPROTO_TCP,
    kUDP = IPPROTO_UDP,
  };
}
}
