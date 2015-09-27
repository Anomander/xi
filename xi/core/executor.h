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

    template < class func > void post(func &&f) {
      _kernel->post(_id, forward< func >(f));
    }

    template < class func > void dispatch(func &&f) {
      _kernel->dispatch(_id, forward< func >(f));
    }

    unsigned id() const { return _id; }
  };
}
}
