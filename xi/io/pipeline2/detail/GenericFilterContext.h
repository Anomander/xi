#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      struct GenericFilterContext : public virtual ownership::StdShared {
        virtual ~GenericFilterContext() = default;
        virtual void addReadIfNull(mut< GenericFilterContext >) = 0;
        virtual void addWriteIfNull(mut< GenericFilterContext >) = 0;
      };
    }
  }
}
}
