#pragma once

#include "xi/ext/configure.h"
#include "xi/core/shard.h"

namespace xi {
namespace core {

  class kernel;

  class executor_pool : public virtual ownership::std_shared {
    vector< mut< shard > > _shards;
    unsigned _next_core = 0;

  public:
    executor_pool(mut< kernel > k, vector< u16 > ids);

    executor_pool(mut< kernel > k, u16 count);

    ~executor_pool();

    size_t size() const;

    template < class F >
    void post_on_all(F &&f) {
      for (auto &e : _shards) {
        e->post(forward< F >(f));
      }
    }

    template < class F >
    void post_on(size_t core, F &&f) {
      executor_for_core(core)->post(forward< F >(f));
    }

    template < class F >
    void post(F &&f) {
      next_executor()->post(forward< F >(f));
    }

    template < class F >
    void dispatch(F &&f) {
      next_executor()->dispatch(forward< F >(f));
    }

    mut< shard > executor_for_core(unsigned id);

    mut< shard > next_executor();
  };
}
}
