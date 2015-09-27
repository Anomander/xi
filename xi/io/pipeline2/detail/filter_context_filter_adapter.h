#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipeline2/modifiers.h"
#include "xi/io/pipeline2/detail/filter_context_impl.h"
#include "xi/io/pipeline2/detail/filter_impl.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      template < class C, class H, class... messages >
      struct filter_context_filter_adapter;

      template < class C, class H, class M0, class... messages >
      struct filter_context_filter_adapter< C, H, M0, messages... >
          : filter_context_impl< C, M0 >,
            filter_context_filter_adapter< C, H, messages... > {
        using super = filter_context_filter_adapter< C, H, messages... >;
        using impl = filter_context_impl< C, M0 >;

      public:
        using super::filter_context_filter_adapter;
        using super::forward_read;
        using super::forward_write;
        using impl::forward_read;
        using impl::forward_write;

        void read(M0 m) override {
          static_cast< mut< filter_impl< C, M0 > > >(this->handler())
              ->read(this->context(), m);
        }
        void write(M0 m) override {
          static_cast< mut< filter_impl< C, M0 > > >(this->handler())
              ->write(this->context(), m);
        }

        void add_read_if_null(mut< generic_filter_context > cx) override {
          read_link_impl< M0 >::add_link_if_null(cx);
          return super::add_read_if_null(cx);
        }

        void add_write_if_null(mut< generic_filter_context > cx) override {
          write_link_impl< M0 >::add_link_if_null(cx);
          return super::add_write_if_null(cx);
        }
      };

      template < class C, class H, class M0, class... messages >
      struct filter_context_filter_adapter< C, H, read_only< M0 >, messages... >
          : filter_context_impl< C, read_only< M0 > >,
            filter_context_filter_adapter< C, H, messages... > {
        using super = filter_context_filter_adapter< C, H, messages... >;
        using impl = filter_context_impl< C, read_only< M0 > >;

      public:
        using super::filter_context_filter_adapter;
        using super::forward_read;
        using super::forward_write;
        using impl::forward_read;

        void read(M0 m) override {
          static_cast< mut< filter_impl< C, read_only< M0 > > > >(
              this->handler())->read(this->context(), m);
        }

        void add_read_if_null(mut< generic_filter_context > cx) override {
          read_link_impl< M0 >::add_link_if_null(cx);
          return super::add_read_if_null(cx);
        }

        void add_write_if_null(mut< generic_filter_context > cx) override {
          return super::add_write_if_null(cx);
        }
      };

      template < class C, class H, class M0, class... messages >
      struct filter_context_filter_adapter< C, H, write_only< M0 >,
                                            messages... >
          : filter_context_impl< C, write_only< M0 > >,
            filter_context_filter_adapter< C, H, messages... > {
        using super = filter_context_filter_adapter< C, H, messages... >;
        using impl = filter_context_impl< C, write_only< M0 > >;

      public:
        using super::filter_context_filter_adapter;
        using super::forward_read;
        using super::forward_write;
        using impl::forward_write;

        void write(M0 m) override {
          static_cast< mut< filter_impl< C, write_only< M0 > > > >(
              this->handler())->write(this->context(), m);
        }

        void add_read_if_null(mut< generic_filter_context > cx) override {
          return super::add_read_if_null(cx);
        }

        void add_write_if_null(mut< generic_filter_context > cx) override {
          write_link_impl< M0 >::add_link_if_null(cx);
          return super::add_write_if_null(cx);
        }
      };

      template < class C, class H >
      struct filter_context_filter_adapter< C, H > : virtual C {
        filter_context_filter_adapter(own< H > h) : _handler(move(h)) {}

        void add_read_if_null(mut< generic_filter_context > cx) {}
        void add_write_if_null(mut< generic_filter_context > cx) {}

      protected:
        mut< H > handler() { return edit(_handler); }
        mut< C > context() { return this; }

      private:
        own< H > _handler = nullptr;
      };
    }
  }
}
}
