#pragma once

#include "xi/ext/configure.h"
#include "xi/core/kernel.h"
#include "xi/io/channel_interface.h"
#include "xi/io/pipes/context_aware_filter.h"
#include "xi/io/pipes/detail/filter_context_filter_adapter.h"
#include "xi/io/pipes/detail/pipe_base.h"
#include "xi/io/pipes/detail/pipe_message_impl.h"

#include "xi/ext/overload_rank.h"

namespace xi {
namespace io {
  namespace pipes {

    namespace detail {

      class pipe_control_interface
          : public virtual detail::pipe_base,
            public virtual detail::pipe_message_impl_base,
            public virtual ownership::std_shared {

        mut< channel_interface > _channel;

      public:
        template < class H >
        void push_front(H);
        template < class H >
        void push_back(H);

        void remove(mut< generic_filter_context >);

        pipe_control_interface(mut< channel_interface > c) : _channel(c) {
        }

      private:
        template < class H, class... M >
        auto make_filter_context(filter< M... > const&, H);

        template < class C >
        void push_front_ctx(C);
        template < class C >
        void push_back_ctx(C);

        template <
            class H,
            class C,
            XI_REQUIRE_DECL(is_base_of< filter_tag< context_aware >, H >) >
        void maybe_pass_context_to_filter(H* f, C* cx, rank< 1 >);
        template < class H, class C >
        void maybe_pass_context_to_filter(H* f, C* cx, rank< 2 >);
      };

      template < class H >
      void pipe_control_interface::push_front(H h) {
        auto mut_h  = edit(h);
        auto cx     = make_filter_context(val(h), move(h));
        auto mut_cx = edit(cx);
        push_front_ctx(move(cx));
        maybe_pass_context_to_filter(mut_h, mut_cx, select_rank);
      }

      template < class H >
      void pipe_control_interface::push_back(H h) {
        auto mut_h  = edit(h);
        auto cx     = make_filter_context(val(h), move(h));
        auto mut_cx = edit(cx);
        push_back_ctx(move(cx));
        maybe_pass_context_to_filter(mut_h, mut_cx, select_rank);
      }

      inline void pipe_control_interface::remove(
          mut< generic_filter_context > cx) {
        maybe_change_head(cx);
        maybe_change_tail(cx);
        defer([this, cx] { pipe_base::remove(cx); });
      }

      template < class C >
      void pipe_control_interface::push_front_ctx(C ctx) {
        auto mut_ctx = edit(ctx);
        pipe_base::push_front(move(ctx));
        maybe_update_head(mut_ctx);
      }

      template < class C >
      void pipe_control_interface::push_back_ctx(C ctx) {
        auto mut_ctx = edit(ctx);
        pipe_base::push_back(move(ctx));
        maybe_update_tail(mut_ctx);
      }

      template < class H, class... M >
      auto pipe_control_interface::make_filter_context(filter< M... > const&,
                                                       H h) {
        auto ctx = make< detail::filter_context_filter_adapter<
            detail::channel_filter_context< M... >,
            H,
            M... > >(move(h));
        ctx->set_pipe(this);
        ctx->set_channel(_channel);
        return ctx;
      }

      template < class H,
                 class C,
                 XI_REQUIRE(is_base_of< filter_tag< context_aware >, H >) >
      void pipe_control_interface::maybe_pass_context_to_filter(H* f,
                                                                C* cx,
                                                                rank< 1 >) {
        f->context_added(cx);
      }

      template < class H, class C >
      void pipe_control_interface::maybe_pass_context_to_filter(H* f,
                                                                C* cx,
                                                                rank< 2 >) {
        // do nothing
      }
    }
  }
}
}
