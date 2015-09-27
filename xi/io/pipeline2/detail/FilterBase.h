#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/detail/FilterImpl.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      template < class Context, class... Messages >
      struct FilterBase;

      template < class Context, class M0, class... Messages >
      struct FilterBase< Context, M0, Messages... > : FilterImpl< Context, M0 >, FilterBase< Context, Messages... > {};

      template < class Context >
      struct FilterBase< Context > {
        virtual ~FilterBase() = default;
      };
    }
  }
}
}
