#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/modifiers.h"
#include "xi/io/pipes/detail/generic_filter_context.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      template < class M >
      struct read_link : virtual generic_filter_context {
        virtual void read(M) = 0;
        virtual mut< read_link > next_read(M* = nullptr) = 0;
      };

      template < class M >
      struct write_link : virtual generic_filter_context {
        virtual void write(M) = 0;
        virtual mut< write_link > next_write(M* = nullptr) = 0;
      };
    }
  }
}
}
