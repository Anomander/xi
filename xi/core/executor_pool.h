#pragma once

#include "xi/ext/configure.h"
#include "xi/core/kernel.h"

namespace xi {
namespace core {

  class kernel;

  class executor_pool : public virtual ownership::std_shared {
    vector< mut< shard > > _shards;
    unsigned _next_core = 0;

  public:
    executor_pool(mut< kernel > k, vector< unsigned > ids) {
      for (auto id : ids) {
        _shards.emplace_back(k->mut_shard(id));
      }
    }

    executor_pool(mut< kernel > k, size_t count) {
      for (u16 id = 0; id < count; ++id) {
        _shards.emplace_back(k->mut_shard(id));
      }
    }

    size_t size() const {
      return _shards.size();
    }

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

    mut< shard > executor_for_core(unsigned id) {
      if (id > _shards.size()) {
        throw std::invalid_argument("Executor with id " + to_string(id) +
                                    " is not registered");
      }
      return _shards[id];
    }

    mut< shard > next_executor() {
      return _shards[(_next_core++) % _shards.size()];
    }
  };
}
}
