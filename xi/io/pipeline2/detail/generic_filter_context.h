#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      struct generic_filter_context : public virtual ownership::std_shared {
        virtual ~generic_filter_context() = default;
        virtual void add_read_if_null(mut< generic_filter_context >) = 0;
        virtual void add_write_if_null(mut< generic_filter_context >) = 0;
      };
    }
  }
}
}
