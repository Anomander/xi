#pragma once

#include "xi/ext/configure.h"
#include "xi/io/pipes/modifiers.h"
#include "xi/io/channel_interface.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      class pipe_control_interface;

      template < class... messages > struct filter_context;

      template < class M0, class... messages >
      struct filter_context< M0, messages... >
          : virtual filter_context< messages... > {
        using filter_context< messages... >::forward_read;
        using filter_context< messages... >::forward_write;

        virtual void forward_read(M0 m) = 0;
        virtual void forward_write(M0 m) = 0;
      };

      template < class M0, class... messages >
      struct filter_context< read_only< M0 >, messages... >
          : virtual filter_context< messages... > {
        using filter_context< messages... >::forward_read;
        using filter_context< messages... >::forward_write;

        virtual void forward_read(M0 m) = 0;
      };

      template < class M0, class... messages >
      struct filter_context< write_only< M0 >, messages... >
          : virtual filter_context< messages... > {
        using filter_context< messages... >::forward_read;
        using filter_context< messages... >::forward_write;

        virtual void forward_write(M0 m) = 0;
      };

      template <> struct filter_context<> {
        mut <pipe_control_interface> _pipe;

      public:
        void forward_read() = delete;
        void forward_write() = delete;

        void set_pipe(mut< pipe_control_interface > p) { _pipe = p; }
        mut< pipe_control_interface > pipe() { return _pipe; }
      };

      template < class... messages >
      struct channel_filter_context : public filter_context< messages... > {
        mut< channel_interface > _channel;

      public:
        void set_channel(mut< channel_interface > c) { _channel = c; }
        mut< channel_interface > channel() { return _channel; }
      };
    }
  }
}
}
