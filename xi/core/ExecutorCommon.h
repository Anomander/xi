#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace core {

  template < class E >
  struct ExecutorCommon : public virtual ownership::StdShared {

    // Returns a function which when called will post the
    // wrapped function into the executor pool.
    // Use this when something wants a runnable argument,
    // but you want to run the execution asynchronously.
    template < class Func >
    auto wrap(Func&& f) {
      return [ f = forward< Func >(f), this ](auto... args) {
        static_cast< E* >(this)->post(::std::bind(f, forward< decltype(args)... >(args)...));
      };
    }
  };
}
}
