#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipeline2/detail/filter_context.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      template < class C, class M > struct filter_impl {
        virtual void read(mut< C > cx, M m) { cx->forward_read(move(m)); }
        virtual void write(mut< C > cx, M m) { cx->forward_write(move(m)); }
      };

      template < class C, class M > struct filter_impl< C, read_only< M > > {
        virtual void read(mut< C > cx, M m) { cx->forward_read(move(m)); }
      };

      template < class C, class M > struct filter_impl< C, write_only< M > > {
        virtual void write(mut< C > cx, M m) { cx->forward_write(move(m)); }
      };
    }
  }
}
}
