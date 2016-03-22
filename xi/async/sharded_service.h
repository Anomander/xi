#pragma once

#include "xi/async/latch.h"
#include "xi/core/executor_pool.h"

namespace xi {
namespace async {

  struct thread_local_service : public virtual ownership::unique {
    virtual ~thread_local_service() = default;
    virtual future<> start()        = 0;
    virtual void stop()             = 0;
  };

  template < class I >
  class sharded_service : public thread_local_service {
    /// TODO: add concept check.
    static thread_local I *_local_impl;

    own< core::executor_pool > _pool;
    vector< own< I > > _implementations;
    latch _start_latch;
    promise<> _stop_promise;

  public:
    sharded_service(own< core::executor_pool > pool)
        : _pool(move(pool)), _start_latch(_pool->size()) {
    }
    ~sharded_service() {
      stop();
    }

  public:
    future<> start() override {
      XI_SCOPE(failure) {
        _implementations.clear();
      };
      for (unsigned i = 0; i < _pool->size(); ++i) {
        _implementations.emplace_back(make< I >());
      }
      for (unsigned i = 0; i < _pool->size(); ++i) {
        std::cout << edit(_implementations[i]) << std::endl;
        _pool->post([ impl = edit(_implementations[i]), this ]() mutable {
          _local_impl = impl;
          impl->start(edit(_pool));
          _start_latch.count_down();
        });
      }
      return _start_latch.await();
    }

    auto await_shutdown() {
      return _stop_promise.get_future();
    }

    void stop() override {
      auto l = make_shared< latch >(_pool->size());
      l->await().then(
          [ impl = move(_implementations), p = move(_stop_promise) ]() mutable {
            impl.clear();
            p.set();
          });
      _pool->post_on_all([ l = move(l), pool = share(_pool) ]() mutable {
        auto *impl = _local_impl;
        if (impl) {
          impl->stop();
        }
        _local_impl = nullptr;
        l->count_down();
      });
    }

    mut< I > local() {
      if (!_local_impl) {
        throw ::std::runtime_error(
            "Local thread is not managing requested service.");
      }
      return _local_impl;
    }

    template < class F >
    void post(F &&func) {
      defer(edit(_pool), forward< F >(func));
    }

    template < class F >
    void post_future(F &&func) {
      defer_future(edit(_pool), forward< F >(func));
    }
  };
  template < class I >
  thread_local I *sharded_service< I >::_local_impl = nullptr;
}
}
