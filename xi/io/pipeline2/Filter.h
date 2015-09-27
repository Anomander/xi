#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/detail/FilterBase.h"

namespace xi {
namespace io {
  namespace pipe2 {

    template < class... Messages >
    struct Filter : public detail::FilterBase< detail::FilterContext< Messages... >, Messages... >,
                    public virtual ownership::StdShared {
      using Context = detail::FilterContext< Messages... >;
      virtual ~Filter() = default;
    };

  }
}
}
