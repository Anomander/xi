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

    struct context_aware; // intentionally incomplete
    template < class T > struct filter_tag {};

    template < class... Ms >
    struct context_aware_filter : public filter< Ms... >,
                                  public filter_tag< context_aware > {
      using context = typename filter< Ms... >::context;
      mut< context > _context = nullptr;

      template < class... > friend class pipe;

      virtual void context_added(mut< context > cx) { _context = cx; }

    protected:
      mut<context> my_context() { return _context; }
    };
  }
}
}
