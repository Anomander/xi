#pragma once

#include "ext/Configure.h"
#include "async/Context.h"
#include "async/Executor.h"

namespace xi {
namespace async {

  template < class Func >
  void schedule(Func &&f) {
    local< Executor >().submit(forward< Func >(f));
  }
}
}
