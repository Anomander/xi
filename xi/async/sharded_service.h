#pragma once

#include "xi/core/executor_pool.h"
#include "xi/async/latch.h"

namespace xi {
namespace async {
  template < class I > class sharded_service : public virtual ownership::unique {
    /// TODO: add concept check.
    own< core::executor_pool > _pool;
    vector< own< I > > _implementations;
    latch _start_latch;

  public:
    sharded_service(own< core::executor_pool > pool)
        : _pool(move(pool)), _start_latch(_pool->size()) {}
    ~sharded_service() { stop(); }

  public:
    auto start() {
      XI_SCOPE(failure) { _implementations.clear(); };
      for (unsigned i = 0; i < _pool->size(); ++i) {
        _implementations.emplace_back(make< I >());
      }
      for (unsigned i = 0; i < _pool->size(); ++i) {
        std::cout << edit(_implementations[i]) << std::endl;
        _pool->post([ impl = edit(_implementations[i]), this ]() mutable {
          set_local< I >(*impl);
          impl->start(edit(_pool));
          _start_latch.count_down();
        });
      }
      return _start_latch.await();
    }

    void stop() {
      auto l = make_shared< latch >(_pool->size());
      l->await().then([impl = move(_implementations)]() mutable {
        impl.clear();
      });
      _pool->post_on_all([ l = move(l), pool = share(_pool) ]() mutable {
        auto *impl = try_local< I >();
        if (impl) { impl->stop(); }
        reset_local< I >();
        l->count_down();
      });
    }

    mut< I > local() {
      auto *impl = try_local< I >();
      if (!impl) {
        throw ::std::runtime_error(
            "Local thread is not managing requested service.");
      }
      return impl;
    }

    template < class F > void post(F &&func) {
      defer(edit(_pool), forward< F >(func));
    }

    template < class F > void post_future(F &&func) {
      defer_future(edit(_pool), forward< F >(func));
    }
  };

}
}
