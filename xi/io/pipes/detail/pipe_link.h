#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/modifiers.h"
#include "xi/io/pipes/detail/links.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      template < class M0 >
      struct pipe_link {
      private:
        mut< read_link< M0 > > _head = nullptr;
        mut< write_link< M0 > > _tail = nullptr;

      protected:
        void maybe_update_head(mut< generic_filter_context > ctx) {
          auto read_m0 = dynamic_cast< mut< read_link< M0 > > >(ctx);
          if (read_m0) {
            _head = read_m0;
          }
          if (!_tail) {
            auto write_m0 = dynamic_cast< mut< write_link< M0 > > >(ctx);
            if (write_m0) {
              _tail = write_m0;
            }
          }
        }

        void maybe_update_tail(mut< generic_filter_context > ctx) {
          auto write_m0 = dynamic_cast< mut< write_link< M0 > > >(ctx);
          if (write_m0) {
            _tail = write_m0;
          }
          if (!_head) {
            auto read_m0 = dynamic_cast< mut< read_link< M0 > > >(ctx);
            if (read_m0) {
              _head = read_m0;
            }
          }
        }
        void maybe_change_head(mut< generic_filter_context > ctx) {
          if (_head == ctx) {
            _head = _head->next_read();
          }
        }
        void maybe_change_tail(mut< generic_filter_context > ctx) {
          if (_tail == ctx) {
            _tail = _tail->next_write();
          }
        }

      public:
        void read(M0 m) {
          if (!_head) {
            return;
          }
          _head->read(move(m));
        }
        void write(M0 m) {
          if (!_tail) {
            return;
          }
          _tail->write(move(m));
        }
      };

      template < class M0 >
      struct pipe_link< read_only< M0 > > {
      private:
        mut< read_link< M0 > > _head = nullptr;

      protected:
        void maybe_update_head(mut< generic_filter_context > ctx) {
          auto read_m0 = dynamic_cast< mut< read_link< M0 > > >(ctx);
          if (read_m0) {
            _head = read_m0;
          }
        }
        void maybe_update_tail(mut< generic_filter_context > ctx) {
          if (!_head) {
            auto read_m0 = dynamic_cast< mut< read_link< M0 > > >(ctx);
            if (read_m0) {
              _head = read_m0;
            }
          }
        }
        void maybe_change_head(mut< generic_filter_context > ctx) {
          if (_head == ctx) {
            _head = _head->next_read();
          }
        }
        void maybe_change_tail(mut< generic_filter_context > ctx) {
        }

      public:
        void read(M0 m) {
          if (!_head) {
            return;
          }
          _head->read(move(m));
        }
        void write(M0 m) = delete;
      };

      template < class M0 >
      struct pipe_link< write_only< M0 > > {
      private:
        mut< write_link< M0 > > _tail = nullptr;

      protected:
        void maybe_update_tail(mut< generic_filter_context > ctx) {
          auto write_m0 = dynamic_cast< mut< write_link< M0 > > >(ctx);
          if (write_m0) {
            _tail = write_m0;
          }
        }
        void maybe_update_head(mut< generic_filter_context > ctx) {
          if (!_tail) {
            auto write_m0 = dynamic_cast< mut< write_link< M0 > > >(ctx);
            if (write_m0) {
              _tail = write_m0;
            }
          }
        }
        void maybe_change_head(mut< generic_filter_context > ctx) {
        }
        void maybe_change_tail(mut< generic_filter_context > ctx) {
          if (_tail == ctx) {
            _tail = _tail->next_write();
          }
        }

      public:
        void write(M0 m) {
          if (!_tail) {
            return;
          }
          _tail->write(move(m));
        }
        void read(M0 m) = delete;
      };
    }
  }
}
}
