#pragma once

#include "xi/ext/Configure.h"
#include "xi/core/Kernel.h"
#include "xi/core/ExecutorCommon.h"

namespace xi {
namespace core {

  class Executor : public ExecutorCommon< Executor > {
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

    unsigned id() const { return _id; }
  };
}
}
