#pragma once

#include "ext/Configure.h"
#include "core/Kernel.h"

namespace xi {
namespace core {

  class Executor {
    mut< Kernel > _kernel;
    unsigned _id;

  public:
    Executor(mut< Kernel > kernel, unsigned id) : _kernel(kernel), _id(id) {}

    template < class Func >
    void post(Func &&f) {
      _kernel->post(_id, forward< Func >(f));
    }

    template < class Func >
    void dispatch(Func &&f) {
      _kernel->dispatch(_id, forward< Func >(f));
    }
  };
}
}
