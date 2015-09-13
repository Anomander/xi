#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace io {
  namespace pipeline {

    template < class E >
    void EventHandlerBase< E >::handleEvent(mut< HandlerContext > cx, own< E > e) {
      cx->forward(move(e));
    }
  }
}
}
