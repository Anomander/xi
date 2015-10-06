#pragma once

#include "xi/core/kernel.h"
#include "xi/core/executor_pool.h"
#include "xi/async/future/promise.h"

namespace xi {
namespace core {

  own< executor_pool > make_executor_pool(mut< kernel > kernel,
                                          vector< unsigned > cores = {}) {
    if (cores.empty()) {
      return make< executor_pool >(kernel, kernel->core_count());
    }
    for (auto id : cores) {
      if (id >= kernel->core_count()) {
        throw std::invalid_argument("Core id not registered: " + to_string(id));
      }
    }
    return make< executor_pool >(kernel, move(cores));
  }

  /// TODO: Add concept checks below
  namespace detail {
    template < class E, class F >
    auto defer_or_dispatch(
        void (E::*method)(F&&), E* executor, F&& func,
        async::promise< async::future_result< F > >&& promise)
        -> async::future< async::future_result< F > > {
      auto f = promise.get_future();
      (executor->*method)(
          [ func = forward< F >(func), p = move(promise) ]() mutable {
            p.apply(move(func));
          });
      return f;
    }
  }
  template < class E, class F > void defer(E* executor, F&& func) {
    executor->post(forward< F >(func));
  }

  template < class E, class F >
  auto defer(E* executor, F&& func,
             async::promise< async::future_result< F > >&& promise) {
    return detail::defer_or_dispatch(&E::post, forward< F >(func),
                                     move(promise));
  }
  template < class E, class F > auto defer_future(E* executor, F&& func) {
    return defer(executor, forward< F >(func),
                 async::promise< async::future_result< F > >{});
  }

  template < class E, class F > void dispatch(E* executor, F&& func) {
    executor->dispatch(forward< F >(func));
  }
  template < class E, class F >
  auto dispatch(E* executor, F&& func,
                async::promise< async::future_result< F > >&& promise) {
    return detail::defer_or_dispatch(&E::dispatch, forward< F >(func),
                                     move(promise));
  }
  template < class E, class F > auto dispatch_future(E* executor, F&& func) {
    return dispatch(executor, forward< F >(func),
                    async::promise< async::future_result< F > >{});
  }
}
}
