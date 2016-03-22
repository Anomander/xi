#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {
  namespace pipes {

    template < class message >
    struct read_only {};
    template < class message >
    struct write_only {};

    template < class message >
    using in = read_only< message >;
    template < class message >
    using out = write_only< message >;
    template < class message >
    using read_write = message;
    template < class message >
    using in_out = message;
  }
}
}
