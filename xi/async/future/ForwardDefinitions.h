#pragma once

#include "xi/ext/Configure.h"
#include "xi/core/Executor.h"
#include "xi/async/future/ForwardDefinitions.h"

namespace xi {
namespace async {

  template < class = meta::Null >
  class Future;
  template < class = meta::Null >
  class Promise;

  template < class U >
  Future< U > makeReadyFuture(U&& value);
  template < class U >
  Future< U > makeReadyFuture(Future< U >&& value);
  Future<> makeReadyFuture();
  Future<> makeReadyFuture(meta::Null);
  template < class U >
  Future< U > makeReadyFuture(exception_ptr ex);
  Future<> makeReadyFuture(exception_ptr ex);

}
}
