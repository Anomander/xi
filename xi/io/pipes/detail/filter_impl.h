#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/modifiers.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      template < class C, class M >
      struct filter_impl {
        virtual void read(mut< C > cx, M m) {
          cx->forward_read(move(m));
        }
        virtual void write(mut< C > cx, M m) {
          cx->forward_write(move(m));
        }
      };

      template < class C, class M >
      struct filter_impl< C, in< M > > {
        virtual void read(mut< C > cx, M m) {
          cx->forward_read(move(m));
        }
      };

      template < class C, class M >
      struct filter_impl< C, out< M > > {
        virtual void write(mut< C > cx, M m) {
          cx->forward_write(move(m));
        }
      };
    }
  }
}
}
