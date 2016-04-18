#pragma once

#include "xi/core/bootstrap.h"
#include "xi/core/executor_pool.h"
#include "xi/core/future/promise.h"

namespace xi {
namespace core {

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
  auto defer(mut< shard >, F&& func, promise< future_result< F > >&& promise) {
    return detail::defer_or_dispatch(
        &shard::post, forward< F >(func), move(promise));
  }
  template < class F >
  auto defer_future(mut< shard > executor, F&& func) {
    return defer(executor, forward< F >(func), promise< future_result< F > >{});
  }

  template < class F >
  void dispatch(mut< shard > executor, F&& func) {
    executor->dispatch(forward< F >(func));
  }

  template < class F >
  auto dispatch(mut< shard >,
                F&& func,
                promise< future_result< F > >&& promise) {
    return detail::defer_or_dispatch(
        &shard::dispatch, forward< F >(func), move(promise));
  }

  template < class F >
  auto dispatch_future(mut< shard > executor, F&& func) {
    return dispatch(
        executor, forward< F >(func), promise< future_result< F > >{});
  }

  template < class F >
  void post_on(u16 core, F&& f) {
    bootstrap::shard_at(core)->post(forward< F >(f));
  }

  template < class F >
  void post_on_all(F const& f) {
    for (auto core : range::to(bootstrap::cpus())) {
      bootstrap::shard_at(core)->post(f /* intentional copy */);
    }
  }
}
}
