#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/filter.h"
#include "xi/io/pipes/detail/pipe_base.h"
#include "xi/io/pipes/detail/pipe_message_impl.h"
#include "xi/io/pipes/detail/filter_context_filter_adapter.h"
#include "xi/io/channel_interface.h"

namespace xi {
namespace io {
  namespace pipes {

    template < class... messages >
    class pipe : public virtual detail::pipe_base,
                 detail::pipe_message_impl< messages... >,
                 public virtual ownership::std_shared {
      using impl = detail::pipe_message_impl< messages... >;

      mut<channel_interface> _channel;
    public:
      pipe(mut<channel_interface> c) : _channel(c) {}

    public:
      using impl::read;
      using impl::write;

      template < class H > void push_front(H);
      template < class H > void push_back(H);

    private:
      template < class H, class... M >
      auto make_filter_context(filter< M... > const &, H);

      template < class C > void push_front_ctx(C);
      template < class C > void push_back_ctx(C);

    };

    template < class... messages >
    template < class H >
    void pipe< messages... >::push_front(H h) {
      push_back_ctx(make_filter_context(val(h), move(h)));
    }

    template < class... messages >
    template < class H >
    void pipe< messages... >::push_back(H h) {
      push_back_ctx (make_filter_context(val(h), move(h)));
    }

    template < class... messages >
    template < class C >
    void pipe< messages... >::push_front_ctx(C ctx) {
      auto mut_ctx = edit(ctx);
      pipe_base::push_front(move(ctx));
      impl::maybe_update_head(mut_ctx);
    }

    template < class... messages >
    template < class C >
    void pipe< messages... >::push_back_ctx(C ctx) {
      auto mut_ctx = edit(ctx);
      pipe_base::push_back(move(ctx));
      impl::maybe_update_tail(mut_ctx);
    }

    template < class... messages >
    template < class H, class... M >
    auto pipe< messages... >::make_filter_context(filter< M... > const &, H h) {
      auto ctx = make< detail::filter_context_filter_adapter<
                         detail::channel_filter_context< M... >, H, M... > >(move(h));
      ctx->set_channel(_channel);
      return ctx;
    }
  }

}
}
