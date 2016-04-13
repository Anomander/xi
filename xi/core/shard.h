#pragma once

#include "xi/ext/lockfree.h"
#include "xi/core/reactor.h"
#include "xi/core/task_queue.h"
#include "xi/util/spin_lock.h"

namespace xi {
namespace core {

  struct alignas(64) poller : public virtual ownership::unique {
    virtual ~poller()                = default;
    virtual unsigned poll() noexcept = 0;
  };
  class deferred_poller;

  class kernel;

  class alignas(64) shard final {
    u16 _core_id = -1;

    struct {
      lockfree::queue< task * > tasks{128};
    } _inbound;

    task_queue _task_queue;
    mut<deferred_poller> _deferred_poller;
    vector< own< poller > > _pollers;
    mut< core::reactor > _reactor;
    mut< kernel > _kernel;

  public:
    shard(mut< kernel >);
    void start();
    void attach_reactor(own< core::reactor >);

  public:
    template < class F >
    void dispatch(F &&func);
    template < class F >
    void post(F &&func);
    // returns a function which when called will post the
    // wrapped function into the executor pool.
    // use this when something wants a runnable argument,
    // but you want to run the execution asynchronously.
    template < class F >
    auto wrap(F &&func);

    usize register_poller(own< poller > poller);
    void deregister_poller(size_t poller_id);

    void poll();

    mut< core::reactor > reactor();

  private:
    void _push_task_to_inbound_queue(own< task >);
    void _post_task(own< task >);
    void _handle_exception();
  };

  extern thread_local mut< shard > this_shard;

  inline mut< core::reactor > shard::reactor() {
    return _reactor;
  }

  template < class F >
  void shard::post(F &&func) {
    _post_task(make_unique_copy(make_task(forward< F >(func))));
  }

  template < class F >
  void shard::dispatch(F &&func) {
    if (this_shard == this) {
      try {
        func();
      } catch (...) {
        _handle_exception();
      }
    } else {
      post(forward< F >(func));
    }
  }

  // returns a function which when called will post the
  // wrapped function into the executor pool.
  // use this when something wants a runnable argument,
  // but you want to run the execution asynchronously.
  template < class F >
  auto shard::wrap(F &&func) {
    return [ f = forward< F >(func), this ](auto... args) {
      post(::std::bind(f, forward< decltype(args)... >(args)...));
    };
  }
}

template < class F >
void
defer(F &&func) {
  assert(nullptr != core::this_shard);
  core::this_shard->post(forward< F >(func));
}

inline mut< core::shard >
shard() {
  assert(core::this_shard);
  return core::this_shard;
}
}
