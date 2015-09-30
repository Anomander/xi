#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/detail/generic_filter_context.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      class pipe_base {
        deque< own< generic_filter_context > > _contexts;

      protected:
        ~pipe_base() = default;
        auto mut_contexts() { return edit(_contexts); }

      public:
        void push_front(own< generic_filter_context > ctx) {
          for (auto &cx : _contexts) { ctx->add_write_if_null(edit(cx)); }
          for (auto &cx : adaptors::reverse(_contexts)) {
            cx->add_read_if_null(edit(ctx));
          }
          _contexts.push_front(move(ctx));
        }

        void push_back(own< generic_filter_context > ctx) {
          for (auto &cx : _contexts) { cx->add_read_if_null(edit(ctx)); }
          for (auto &cx : adaptors::reverse(_contexts)) {
            ctx->add_write_if_null(edit(cx));
          }
          _contexts.push_back(move(ctx));
        }
      };
    }
  }
}
}
