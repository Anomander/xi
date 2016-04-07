#pragma once

#include "xi/ext/common.h"

#include <mutex>

namespace xi {
inline namespace ext {

  namespace once {
    using flag = ::std::once_flag;

    template < class... A >
    auto call(flag& f, A&&... a) {
      return ::std::call_once(f, forward< A >(a)...);
    }

    template < class F >
    auto call_lambda(F&& lambda) {
      static flag f;
      return ::std::call_once(f, forward< F >(lambda));
    }
  }

} // inline namespace ext
} // namespace xi
