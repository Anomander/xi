#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/detail/FilterContext.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      template < class C, class M >
      struct FilterImpl {
        virtual void read(mut< C > cx, M m) { cx->forwardRead(move(m)); }
        virtual void write(mut< C > cx, M m) { cx->forwardWrite(move(m)); }
      };

      template < class C, class M >
      struct FilterImpl< C, ReadOnly< M > > {
        virtual void read(mut< C > cx, M m) { cx->forwardRead(move(m)); }
      };

      template < class C, class M >
      struct FilterImpl< C, WriteOnly< M > > {
        virtual void write(mut< C > cx, M m) { cx->forwardWrite(move(m)); }
      };
    }
  }
}
}
