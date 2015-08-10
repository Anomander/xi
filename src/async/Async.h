#pragma once

#include "ext/Configure.h"
#include "async/Context.h"
#include "async/Executor.h"

namespace xi {
namespace async {

  struct InvalidStateException : public std::logic_error {
    using std::logic_error::logic_error;
  };

  template < class Func >
  void schedule(Func &&f) {
    auto * exec = tryLocal<Executor>();
    if (! exec) {
      throw InvalidStateException {"Can't schedule tasks from a foreign thread."};
    }
    exec->submit(forward< Func >(f));
  }
}
}
