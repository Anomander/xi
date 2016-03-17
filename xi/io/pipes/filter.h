#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/detail/filter_base.h"
#include "xi/io/pipes/detail/filter_context.h"

namespace xi {
namespace io {
  namespace pipes {

    template < class... Ms >
    struct filter
        : public detail::filter_base< detail::channel_filter_context< Ms... >,
                                      Ms... >,
          public virtual ownership::std_shared {

    public:
      using context = detail::channel_filter_context< Ms... >;
      virtual ~filter() = default;
    };

  }
}
}
