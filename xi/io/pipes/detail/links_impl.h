#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/detail/links.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      template < class M >
      struct read_link_impl : virtual read_link< M > {
        mut< read_link< M > > next_read(M* = nullptr) final override {
          return _next_read;
        }

        void add_link_if_null(mut< generic_filter_context > cx) {
          auto read_m = dynamic_cast< mut< read_link< M > > >(cx);
          if (read_m && nullptr == _next_read) {
            _next_read = read_m;
          }
        }

        void unlink_read(mut< generic_filter_context > cx) override {
          if (_next_read == cx && cx) {
            _next_read = _next_read->next_read();
          }
        }

      private:
        mut< read_link< M > > _next_read = nullptr;
      };

      template < class M >
      struct write_link_impl : virtual write_link< M > {
        mut< write_link< M > > next_write(M* = nullptr) final override {
          return _next_write;
        }

        void add_link_if_null(mut< generic_filter_context > cx) {
          auto write_m = dynamic_cast< mut< write_link< M > > >(cx);
          if (write_m && nullptr == _next_write) {
            _next_write = write_m;
          }
        }

        void unlink_write(mut< generic_filter_context > cx) override {
          if (_next_write == cx && cx) {
            _next_write = _next_write->next_write();
          }
        }

      private:
        mut< write_link< M > > _next_write = nullptr;
      };
    }
  }
}
}
