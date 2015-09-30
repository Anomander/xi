#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/modifiers.h"
#include "xi/io/pipes/detail/links_impl.h"
#include "xi/io/pipes/detail/filter_context.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      template < class C, class M0 >
      struct filter_context_impl : write_link_impl< M0 >,
                                   read_link_impl< M0 >,
                                   virtual C {

        void forward_read(M0 m) final override {
          auto next_read =
              static_cast< read_link_impl< M0 > * >(this)->next_read();
          if (next_read) { next_read->read(move(m)); }
        }
        void forward_write(M0 m) final override {
          auto next_write =
              static_cast< write_link_impl< M0 > * >(this)->next_write();
          if (next_write) { next_write->write(move(m)); }
        }
      };

      template < class C, class M0 >
      struct filter_context_impl< C, read_only< M0 > > : read_link_impl< M0 >,
                                                         virtual C {

        void forward_read(M0 m) final override {
          auto next_read =
              static_cast< read_link_impl< M0 > * >(this)->next_read();
          if (next_read) { next_read->read(move(m)); }
        }
      };

      template < class C, class M0 >
      struct filter_context_impl< C, write_only< M0 > > : write_link_impl< M0 >,
                                                          virtual C {

        void forward_write(M0 m) final override {
          auto next_write =
              static_cast< write_link_impl< M0 > * >(this)->next_write();
          if (next_write) { next_write->write(move(m)); }
        }
      };
    }
  }
}
}
