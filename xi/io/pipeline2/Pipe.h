#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/Filter.h"
#include "xi/io/pipeline2/detail/PipeBase.h"
#include "xi/io/pipeline2/detail/PipeMessageImpl.h"
#include "xi/io/pipeline2/detail/FilterContextFilterAdapter.h"

namespace xi {
namespace io {
  namespace pipe2 {

    template < class... Messages >
    class Pipe : public virtual detail::PipeBase,
                 detail::PipeMessageImpl< Messages... >,
                 public virtual ownership::StdShared {
      using Impl = detail::PipeMessageImpl< Messages... >;

    public:
      using Impl::read;
      using Impl::write;

      template < class H >
      void pushFront(H);

      template < class H >
      void pushBack(H);

    private:
      template < class H, class... M >
      auto makeFilterContext(Filter< M... > const&, H);
    };

    template < class... Messages >
    template < class H >
    void Pipe< Messages... >::pushFront(H h) {
      auto ctx = makeFilterContext(val(h), move(h));
      auto mutCtx = edit(ctx);
      PipeBase::pushFront(move(ctx));
      Impl::maybeUpdateHead(mutCtx);
    }

    template < class... Messages >
    template < class H >
    void Pipe< Messages... >::pushBack(H h) {
      auto ctx = makeFilterContext(val(h), move(h));
      auto mutCtx = edit(ctx);
      PipeBase::pushBack(move(ctx));
      Impl::maybeUpdateTail(mutCtx);
    }

    template < class... Messages >
    template < class H, class... M >
    auto Pipe< Messages... >::makeFilterContext(Filter< M... > const&, H h) {
      return make< detail::FilterContextFilterAdapter< detail::FilterContext< M... >, H, M... > >(move(h));
    }
  }
}
}
