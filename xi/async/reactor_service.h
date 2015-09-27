#pragma once

#include "xi/core/executor_pool.h"
#include "xi/async/latch.h"

namespace xi {
namespace async {
  template < class I > class service : public virtual ownership::unique {
    /// TODO: add concept check.
    own< core::executor_pool > _pool;
    vector< own< I > > _implementations;
    latch _start_latch;

  public:
    service(own< core::executor_pool > pool)
        : _pool(move(pool)), _start_latch(_pool->size()) {}
    ~service() { stop(); }

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
  };

  template < class R > class reactor_poller : public core::poller {
    mut< R > _reactor;

  public:
    reactor_poller(mut< R > reactor) : _reactor(reactor) {}
    unsigned poll() noexcept override {
      try {
        _reactor->poll();
      } catch (...) {}
      return 0;
    }
  };

  template < class R >
  class reactor_service : public virtual ownership::std_shared {
    own< R > _reactor = make< R >();
    size_t _poller_id = 0;
    mut< core::executor_pool > _pool;

  public:
    void start(mut< core::executor_pool > pool) {
      auto maybe_local_executor = pool->share_local_executor();
      if (!maybe_local_executor) {
        throw std::logic_error("No local executor available.");
      }
      _reactor->attach_executor(move(maybe_local_executor.get()));
      _pool = pool;
      _poller_id =
          pool->register_poller(make< reactor_poller< R > >(edit(_reactor)));
    }
    void stop() {
      _reactor->detach_executor();
      _pool->deregister_poller(_poller_id);
    }
    mut< R > reactor() { return edit(_reactor); }
  };
}
}
