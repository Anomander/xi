#pragma once

#include "xi/ext/configure.h"
#include "xi/core/kernel.h"
#include "xi/core/executor_common.h"

namespace xi {
namespace core {

  class executor : public executor_common< executor > {
    mut< kernel > _kernel;
    unsigned _id;

  public:
    executor(mut< kernel > kernel, unsigned id) : _kernel(kernel), _id(id) {}

    template < class F > void post(F &&f) {
      _kernel->post(_id, forward< F >(f));
    }

    template < class F > void dispatch(F &&f) {
      _kernel->dispatch(_id, forward< F >(f));
    }

    unsigned id() const { return _id; }
  };
}
}
