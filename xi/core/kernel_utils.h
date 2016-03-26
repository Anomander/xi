#pragma once

#include "xi/core/executor_pool.h"
#include "xi/core/future/promise.h"
#include "xi/core/kernel.h"

namespace xi {
namespace core {

  own< executor_pool > make_executor_pool(mut< kernel > kernel,
                                          vector< u16 > cores = {}) {
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
    auto defer_or_dispatch(void (E::*method)(F&&),
                           E* executor,
                           F&& func,
                           promise< future_result< F > >&& promise)
        -> future< future_result< F > > {
      auto f = promise.get_future();
      (executor->*method)(
          [ func = forward< F >(func), p = move(promise) ]() mutable {
            p.apply(move(func));
          });
      return f;
    }
  }

  template < class F >
  auto defer(mut<shard> executor, F&& func, promise< future_result< F > >&& promise) {
    return detail::defer_or_dispatch(
        &shard::post, forward< F >(func), move(promise));
  }
  template < class F >
  auto defer_future(mut<shard> executor, F&& func) {
    return defer(executor, forward< F >(func), promise< future_result< F > >{});
  }

  template < class F >
  void dispatch(mut<shard> executor, F&& func) {
    executor->dispatch(forward< F >(func));
  }
  template < class F >
  auto dispatch(mut<shard> executor,
                F&& func,
                promise< future_result< F > >&& promise) {
    return detail::defer_or_dispatch(
        &shard::dispatch, forward< F >(func), move(promise));
  }
  template < class F >
  auto dispatch_future(mut<shard> executor, F&& func) {
    return dispatch(
        executor, forward< F >(func), promise< future_result< F > >{});
  }
}
}
