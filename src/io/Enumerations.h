#pragma once

#include <netinet/in.h>

namespace xi {
namespace io {

enum AddressFamily {
  kInet = AF_INET,
  kInet6 = AF_INET6,
  kUnix = AF_UNIX,
};

enum SocketType {
  kStream = SOCK_STREAM,
  kDatagram = SOCK_DGRAM,
};

enum Protocol {
  kNone = 0,
  kTCP = IPPROTO_TCP,
  kUDP = IPPROTO_UDP,
};
}
}
