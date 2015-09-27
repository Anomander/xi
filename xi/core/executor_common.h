#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {

  template < class E >
  struct executor_common : public virtual ownership::std_shared {

    // returns a function which when called will post the
    // wrapped function into the executor pool.
    // use this when something wants a runnable argument,
    // but you want to run the execution asynchronously.
    template < class func > auto wrap(func &&f) {
      return [ f = forward< func >(f), this ](auto... args) {
        static_cast< E * >(this)
            ->post(::std::bind(f, forward< decltype(args)... >(args)...));
      };
    }
  };
}
}
