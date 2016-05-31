#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pollable_fd.h"
#include "xi/util/result.h"

#include <sys/socket.h>

namespace xi {
namespace io {
  namespace net {
    class socket : public port {
    public:
      using port::port;

      int read_some(char* data, size_t size);
      int write_some(char* data, size_t size);
      size_t write_exactly(char* data, size_t size);
      template < class O >
      void set_option(O option);
    };

    template < class O >
    inline void socket::set_option(O option) {
      auto value = option.value();
      ::setsockopt(fd(), O::level, O::name, &value, O::length);
    }

  }
}
}
