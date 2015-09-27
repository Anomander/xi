#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipeline2/filter.h"
#include "xi/io/pipeline2/detail/pipe_base.h"
#include "xi/io/pipeline2/detail/pipe_message_impl.h"
#include "xi/io/pipeline2/detail/filter_context_filter_adapter.h"

namespace xi {
namespace io {
  namespace pipe2 {

    template < class... messages >
    class pipe : public virtual detail::pipe_base,
                 detail::pipe_message_impl< messages... >,
                 public virtual ownership::std_shared {
      using impl = detail::pipe_message_impl< messages... >;

    public:
      using impl::read;
      using impl::write;

      template < class H > void push_front(H);

      template < class H > void push_back(H);

    private:
      template < class H, class... M >
      auto make_filter_context(filter< M... > const &, H);
    };

    template < class... messages >
    template < class H >
    void pipe< messages... >::push_front(H h) {
      auto ctx = make_filter_context(val(h), move(h));
      auto mut_ctx = edit(ctx);
      pipe_base::push_front(move(ctx));
      impl::maybe_update_head(mut_ctx);
    }

    template < class... messages >
    template < class H >
    void pipe< messages... >::push_back(H h) {
      auto ctx = make_filter_context(val(h), move(h));
      auto mut_ctx = edit(ctx);
      pipe_base::push_back(move(ctx));
      impl::maybe_update_tail(mut_ctx);
    }

    template < class... messages >
    template < class H, class... M >
    auto pipe< messages... >::make_filter_context(filter< M... > const &, H h) {
      return make< detail::filter_context_filter_adapter<
          detail::filter_context< M... >, H, M... > >(move(h));
    }
  }
}
}
