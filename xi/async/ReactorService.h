#pragma once

#include "xi/core/ExecutorPool.h"
#include "xi/async/Latch.h"

namespace xi {
namespace async {
  template < class I >
  class Service : public virtual ownership::Unique {
    /// TODO: Add concept check.
    own< core::ExecutorPool > _pool;
    vector< own< I > > _implementations;
    Latch _startLatch;

  public:
    Service(own< core::ExecutorPool > pool) : _pool(move(pool)), _startLatch(_pool->size()) {}
    ~Service() { stop(); }

  public:
    auto start() {
      XI_SCOPE(failure) { _implementations.clear(); };
      for (unsigned i = 0; i < _pool->size(); ++i) {
        _implementations.emplace_back(make< I >());
      }
      for (unsigned i = 0; i < _pool->size(); ++i) {
        std::cout << edit(_implementations[i]) << std::endl;
        _pool->post([ impl = edit(_implementations[i]), this ]() mutable {
          setLocal< I >(*impl);
          impl->start(edit(_pool));
          _startLatch.countDown();
        });
      }
      return _startLatch.await();
    }

    void stop() {
      auto latch = make_shared< Latch >(_pool->size());
      latch->await().then([impl = move(_implementations)]() mutable { impl.clear(); });
      _pool->postOnAll([ latch = move(latch), pool = share(_pool) ]() mutable {
        auto* impl = tryLocal< I >();
        if (impl) {
          impl->stop();
        }
        resetLocal< I >();
        latch->countDown();
      });
    }

    mut< I > local() {
      auto* impl = tryLocal< I >();
      if (!impl) {
        throw ::std::runtime_error("Local thread is not managing requested service.");
      }
      return impl;
    }
  };

  template < class R >
  class ReactorPoller : public core::Poller {
    mut< R > _reactor;

  public:
    ReactorPoller(mut< R > reactor) : _reactor(reactor) {}
    unsigned poll() noexcept override {
      try {
        _reactor->poll();
      } catch (...) {
      }
      return 0;
    }
  };

  template < class R >
  class ReactorService : public virtual ownership::StdShared {
    own< R > _reactor = make< R >();
    size_t _pollerId = 0;
    mut< core::ExecutorPool > _pool;

  public:
    void start(mut< core::ExecutorPool > pool) {
      _pool = pool;
      _pollerId = pool->registerPoller(make< ReactorPoller< R > >(edit(_reactor)));
    }
    void stop() { _pool->deregisterPoller(_pollerId); }
    mut< R > reactor() { return edit(_reactor); }
  };
}
}
