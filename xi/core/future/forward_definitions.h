#pragma once

#include "xi/ext/configure.h"
#include "xi/core/future/forward_definitions.h"
#include "xi/core/shard.h"

namespace xi {
namespace core {

  template < class = meta::null >
  class future;
  template < class = meta::null >
  class promise;

  template < class U >
  future< U > make_ready_future(U &&value);

  template < class U >
  future< U > make_ready_future(future< U > &&value);

  future<> make_ready_future();

  future<> make_ready_future(meta::null);

  template < class U >
  future< U > make_ready_future(exception_ptr ex);

  future<> make_ready_future(exception_ptr ex);
}
}
