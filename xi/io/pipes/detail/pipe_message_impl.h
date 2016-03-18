#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/detail/pipe_link.h"
#include "xi/io/pipes/detail/generic_filter_context.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      template < class... messages >
      struct pipe_message_impl;

      template < class M0, class... messages >
      struct pipe_message_impl< M0, messages... >
          : pipe_link< M0 >, pipe_message_impl< messages... > {
        using super = pipe_message_impl< messages... >;
        using pipe_link< M0 >::read;
        using pipe_link< M0 >::write;

        using super::read;
        using super::write;

      protected:
        void maybe_update_head(mut< generic_filter_context > ctx) override {
          pipe_link< M0 >::maybe_update_head(ctx);
          return super::maybe_update_head(ctx);
        }
        void maybe_update_tail(mut< generic_filter_context > ctx) override {
          pipe_link< M0 >::maybe_update_tail(ctx);
          return super::maybe_update_tail(ctx);
        }
        void maybe_change_head(mut< generic_filter_context > ctx) override {
          pipe_link< M0 >::maybe_change_head(ctx);
          return super::maybe_change_head(ctx);
        }
        void maybe_change_tail(mut< generic_filter_context > ctx) override {
          pipe_link< M0 >::maybe_change_tail(ctx);
          return super::maybe_change_tail(ctx);
        }
      };

      struct pipe_message_impl_base {
        virtual void maybe_change_head(mut< generic_filter_context > ctx) {
        }
        virtual void maybe_change_tail(mut< generic_filter_context > ctx) {
        }
        virtual void maybe_update_head(mut< generic_filter_context > ctx) {
        }
        virtual void maybe_update_tail(mut< generic_filter_context > ctx) {
        }
      };

      template <>
      struct pipe_message_impl<> : public virtual pipe_message_impl_base {
      protected:
        void maybe_change_head(mut< generic_filter_context > ctx) override {
        }
        void maybe_change_tail(mut< generic_filter_context > ctx) override {
        }
        void maybe_update_head(mut< generic_filter_context > ctx) override {
        }
        void maybe_update_tail(mut< generic_filter_context > ctx) override {
        }
        void read() = delete;
        void write() = delete;
      };
    }
  }
}
}
