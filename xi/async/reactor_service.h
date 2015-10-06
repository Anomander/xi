#pragma once

#include "xi/core/kernel.h"
#include "xi/core/executor_pool.h"

namespace xi {
namespace async {

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
      if (maybe_local_executor.is_none()) {
        throw std::logic_error("No local executor available.");
      }
      _reactor->attach_executor(maybe_local_executor.unwrap());
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
