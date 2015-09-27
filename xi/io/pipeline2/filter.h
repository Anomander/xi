#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipeline2/detail/filter_base.h"

namespace xi {
namespace io {
  namespace pipe2 {

    template < class... messages >
    struct filter : public detail::filter_base<
                        detail::filter_context< messages... >, messages... >,
                    public virtual ownership::std_shared {
      using context = detail::filter_context< messages... >;
      virtual ~filter() = default;
    };
  }
}
}
