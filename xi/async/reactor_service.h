#pragma once

#include "xi/core/executor_pool.h"
#include "xi/core/kernel.h"

namespace xi {
namespace async {

  template < class R >
  class reactor_poller : public core::poller {
    mut< R > _reactor;

  public:
    reactor_poller(mut< R > reactor) : _reactor(reactor) {
    }
    unsigned poll() noexcept override {
      try {
        _reactor->poll();
      } catch (...) {
      }
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
      _reactor->attach_shard(core::this_shard);
      _pool      = pool;
      _poller_id = core::this_shard->register_poller(
          make< reactor_poller< R > >(edit(_reactor)));
    }
    void stop() {
      core::this_shard->deregister_poller(_poller_id);
      _reactor.reset();
    }
    mut< R > reactor() {
      return edit(_reactor);
    }
  };
}
}
