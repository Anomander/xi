#pragma once

#include "xi/io/pipes/detail/pipe_control_interface.h"
#include "xi/io/pipes/detail/pipe_message_impl.h"

namespace xi {
namespace io {
  namespace pipes {

    template < class... messages >
    class pipe : public detail::pipe_control_interface,
                 private detail::pipe_message_impl< messages... > {
      using impl = detail::pipe_message_impl< messages... >;

    public:
      using pipe_control_interface::pipe_control_interface;

    public:
      using impl::read;
      using impl::write;
    };
  }
}
}
