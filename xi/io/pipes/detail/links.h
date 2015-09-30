#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/modifiers.h"
#include "xi/io/pipes/detail/generic_filter_context.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      template < class message >
      struct read_link : virtual generic_filter_context {
        virtual void read(message) = 0;
      };

      template < class message >
      struct write_link : virtual generic_filter_context {
        virtual void write(message) = 0;
      };
    }
  }
}
}
