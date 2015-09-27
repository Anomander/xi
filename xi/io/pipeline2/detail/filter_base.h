#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipeline2/detail/filter_impl.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      template < class context, class... messages > struct filter_base;

      template < class context, class M0, class... messages >
      struct filter_base< context, M0, messages... >
          : filter_impl< context, M0 >, filter_base< context, messages... > {};

      template < class context > struct filter_base< context > {
        virtual ~filter_base() = default;
      };
    }
  }
}
}
