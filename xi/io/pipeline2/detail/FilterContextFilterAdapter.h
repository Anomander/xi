#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/Modifiers.h"
#include "xi/io/pipeline2/detail/FilterContextImpl.h"
#include "xi/io/pipeline2/detail/FilterImpl.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      template < class C, class H, class... Messages >
      struct FilterContextFilterAdapter;

      template < class C, class H, class M0, class... Messages >
      struct FilterContextFilterAdapter< C, H, M0, Messages... > : FilterContextImpl< C, M0 >,
                                                                   FilterContextFilterAdapter< C, H, Messages... > {
        using Super = FilterContextFilterAdapter< C, H, Messages... >;
        using Impl = FilterContextImpl< C, M0 >;

      public:
        using Super::FilterContextFilterAdapter;
        using Super::forwardRead;
        using Super::forwardWrite;
        using Impl::forwardRead;
        using Impl::forwardWrite;

        void read(M0 m) override {
          static_cast< mut< FilterImpl< C, M0 > > >(this->handler())->read(this->context(), m);
        }
        void write(M0 m) override {
          static_cast< mut< FilterImpl< C, M0 > > >(this->handler())->write(this->context(), m);
        }

        void addReadIfNull(mut< GenericFilterContext > cx) override {
          ReadLinkImpl< M0 >::addLinkIfNull(cx);
          return Super::addReadIfNull(cx);
        }

        void addWriteIfNull(mut< GenericFilterContext > cx) override {
          WriteLinkImpl< M0 >::addLinkIfNull(cx);
          return Super::addWriteIfNull(cx);
        }
      };

      template < class C, class H, class M0, class... Messages >
      struct FilterContextFilterAdapter< C, H, ReadOnly< M0 >, Messages... >
          : FilterContextImpl< C, ReadOnly< M0 > >, FilterContextFilterAdapter< C, H, Messages... > {
        using Super = FilterContextFilterAdapter< C, H, Messages... >;
        using Impl = FilterContextImpl< C, ReadOnly< M0 > >;

      public:
        using Super::FilterContextFilterAdapter;
        using Super::forwardRead;
        using Super::forwardWrite;
        using Impl::forwardRead;

        void read(M0 m) override {
          static_cast< mut< FilterImpl< C, ReadOnly< M0 > > > >(this->handler())->read(this->context(), m);
        }

        void addReadIfNull(mut< GenericFilterContext > cx) override {
          ReadLinkImpl< M0 >::addLinkIfNull(cx);
          return Super::addReadIfNull(cx);
        }

        void addWriteIfNull(mut< GenericFilterContext > cx) override { return Super::addWriteIfNull(cx); }
      };

      template < class C, class H, class M0, class... Messages >
      struct FilterContextFilterAdapter< C, H, WriteOnly< M0 >, Messages... >
          : FilterContextImpl< C, WriteOnly< M0 > >, FilterContextFilterAdapter< C, H, Messages... > {
        using Super = FilterContextFilterAdapter< C, H, Messages... >;
        using Impl = FilterContextImpl< C, WriteOnly< M0 > >;

      public:
        using Super::FilterContextFilterAdapter;
        using Super::forwardRead;
        using Super::forwardWrite;
        using Impl::forwardWrite;

        void write(M0 m) override {
          static_cast< mut< FilterImpl< C, WriteOnly< M0 > > > >(this->handler())->write(this->context(), m);
        }

        void addReadIfNull(mut< GenericFilterContext > cx) override { return Super::addReadIfNull(cx); }

        void addWriteIfNull(mut< GenericFilterContext > cx) override {
          WriteLinkImpl< M0 >::addLinkIfNull(cx);
          return Super::addWriteIfNull(cx);
        }
      };

      template < class C, class H >
      struct FilterContextFilterAdapter< C, H > : virtual C {
        FilterContextFilterAdapter(own< H > h) : _handler(move(h)) {}

        void addReadIfNull(mut< GenericFilterContext > cx) {}
        void addWriteIfNull(mut< GenericFilterContext > cx) {}

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
