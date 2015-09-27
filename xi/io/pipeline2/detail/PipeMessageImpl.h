#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/detail/PipeLink.h"
#include "xi/io/pipeline2/detail/GenericFilterContext.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

    template < class... Messages >
    struct PipeMessageImpl;

    template < class M0, class... Messages >
    struct PipeMessageImpl< M0, Messages... > : PipeLink< M0 >, PipeMessageImpl< Messages... > {
      using Super = PipeMessageImpl< Messages... >;
      using PipeLink< M0 >::read;
      using PipeLink< M0 >::write;

      using Super::read;
      using Super::write;

    protected:
      void maybeUpdateHead(mut< GenericFilterContext > ctx) {
        PipeLink< M0 >::maybeUpdateHead(ctx);
        return Super::maybeUpdateHead(ctx);
      }
      void maybeUpdateTail(mut< GenericFilterContext > ctx) {
        PipeLink< M0 >::maybeUpdateTail(ctx);
        return Super::maybeUpdateTail(ctx);
      }
    };

    template <>
    struct PipeMessageImpl<> {
    protected:
      void maybeUpdateHead(mut< GenericFilterContext > ctx) {}
      void maybeUpdateTail(mut< GenericFilterContext > ctx) {}
      void read() = delete;
      void write() = delete;
    };

  }
}
}
}
