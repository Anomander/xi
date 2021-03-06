#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/detail/filter_context_impl.h"
#include "xi/io/pipes/detail/filter_impl.h"
#include "xi/io/pipes/modifiers.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      template < class C, class H, class... messages >
      struct filter_context_filter_adapter;

      template < class C, class H, class M0, class... messages >
      struct filter_context_filter_adapter< C, H, M0, messages... >
          : filter_context_impl< C, M0 >,
            filter_context_filter_adapter< C, H, messages... > {
        using super = filter_context_filter_adapter< C, H, messages... >;
        using impl  = filter_context_impl< C, M0 >;

      public:
        using super::super;
        using super::forward_read;
        using super::forward_write;
        using impl::forward_read;
        using impl::forward_write;

        void read(M0 m) override {
          static_cast< mut< filter_impl< C, M0 > > >(this->handler())
              ->read(this->context(), move(m));
        }

        void write(M0 m) override {
          static_cast< mut< filter_impl< C, M0 > > >(this->handler())
              ->write(this->context(), move(m));
        }

        void add_read_if_null(mut< generic_filter_context > cx) override {
          read_link_impl< M0 >::add_link_if_null(cx);
          return super::add_read_if_null(cx);
        }

        void add_write_if_null(mut< generic_filter_context > cx) override {
          write_link_impl< M0 >::add_link_if_null(cx);
          return super::add_write_if_null(cx);
        }

        void unlink_read(mut< generic_filter_context > cx) override {
          read_link_impl< M0 >::unlink_read(cx);
          return super::unlink_read(cx);
        }

        void unlink_write(mut< generic_filter_context > cx) override {
          write_link_impl< M0 >::unlink_write(cx);
          return super::unlink_write(cx);
        }
      };

      template < class C, class H, class M0, class... messages >
      struct filter_context_filter_adapter< C, H, read_only< M0 >, messages... >
          : filter_context_impl< C, read_only< M0 > >,
            filter_context_filter_adapter< C, H, messages... > {
        using super = filter_context_filter_adapter< C, H, messages... >;
        using impl  = filter_context_impl< C, read_only< M0 > >;

      public:
        using super::super;
        using super::forward_read;
        using super::forward_write;
        using impl::forward_read;

        void read(M0 m) override {
          static_cast< mut< filter_impl< C, read_only< M0 > > > >(
              this->handler())
              ->read(this->context(), move(m));
        }

        void add_read_if_null(mut< generic_filter_context > cx) override {
          read_link_impl< M0 >::add_link_if_null(cx);
          return super::add_read_if_null(cx);
        }

        void add_write_if_null(mut< generic_filter_context > cx) override {
          return super::add_write_if_null(cx);
        }

        void unlink_read(mut< generic_filter_context > cx) override {
          read_link_impl< M0 >::unlink_read(cx);
          return super::unlink_read(cx);
        }

        void unlink_write(mut< generic_filter_context > cx) override {
          return super::unlink_write(cx);
        }
      };

      template < class C, class H, class M0, class... messages >
      struct filter_context_filter_adapter< C,
                                            H,
                                            write_only< M0 >,
                                            messages... >
          : filter_context_impl< C, write_only< M0 > >,
            filter_context_filter_adapter< C, H, messages... > {
        using super = filter_context_filter_adapter< C, H, messages... >;
        using impl  = filter_context_impl< C, write_only< M0 > >;

      public:
        using super::super;
        using super::forward_read;
        using super::forward_write;
        using impl::forward_write;

        void write(M0 m) override {
          static_cast< mut< filter_impl< C, write_only< M0 > > > >(
              this->handler())
              ->write(this->context(), move(m));
        }

        void add_read_if_null(mut< generic_filter_context > cx) override {
          return super::add_read_if_null(cx);
        }

        void add_write_if_null(mut< generic_filter_context > cx) override {
          write_link_impl< M0 >::add_link_if_null(cx);
          return super::add_write_if_null(cx);
        }

        void unlink_read(mut< generic_filter_context > cx) override {
          return super::unlink_read(cx);
        }

        void unlink_write(mut< generic_filter_context > cx) override {
          write_link_impl< M0 >::unlink_write(cx);
          return super::unlink_write(cx);
        }
      };

      template < class C, class H >
      struct filter_context_filter_adapter< C, H > : virtual C {
        filter_context_filter_adapter(own< H > h) : _handler(move(h)) {
        }

        void add_read_if_null(mut< generic_filter_context >) override {
        }
        void add_write_if_null(mut< generic_filter_context >) override {
        }

        void unlink_read(mut< generic_filter_context >) override {
        }
        void unlink_write(mut< generic_filter_context >) override {
        }

      protected:
        mut< H > handler() {
          return edit(_handler);
        }
        mut< C > context() {
          return this;
        }

      private:
        own< H > _handler = nullptr;
      };
    }
  }
}
}
