#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/detail/GenericFilterContext.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

    class PipeBase {
      deque< own< GenericFilterContext > > _contexts;

    protected:
      ~PipeBase() = default;
      auto mutContexts() { return edit(_contexts); }

    public:
      void pushFront(own< GenericFilterContext > ctx) {
        for (auto& cx : _contexts) {
          ctx->addWriteIfNull(edit(cx));
        }
        for (auto& cx : adaptors::reverse(_contexts)) {
          cx->addReadIfNull(edit(ctx));
        }
        _contexts.push_front(move(ctx));
      }

      void pushBack(own< GenericFilterContext > ctx) {
        for (auto& cx : _contexts) {
          cx->addReadIfNull(edit(ctx));
        }
        for (auto& cx : adaptors::reverse(_contexts)) {
          ctx->addWriteIfNull(edit(cx));
        }
        _contexts.push_back(move(ctx));
      }
    };

  }
}
}
}
