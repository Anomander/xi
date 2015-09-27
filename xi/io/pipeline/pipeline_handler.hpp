#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {
  namespace pipeline {

    template < class E >
    void event_handler_base< E >::handle_event(mut< handler_context > cx,
                                               own< E > e) {
      cx->forward(move(e));
    }
  }
}
}
