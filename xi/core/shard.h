#pragma once

#include "xi/ext/lockfree.h"
#include "xi/core/reactor.h"
#include "xi/core/task_queue.h"

namespace xi {
namespace core {

  class message_bus;
  class deferred_poller;

  struct alignas(64) poller : public virtual ownership::unique {
    virtual ~poller()                = default;
    virtual unsigned poll() noexcept = 0;
  };

  class alignas(64) shard final {
    u16 _core_id = -1;

    struct {
      lockfree::queue< task * > tasks{128};
    } _inbound;

    class signals;

    volatile bool _running = false;
    task_queue _task_queue;
    mut< deferred_poller > _deferred_poller;
    mut< signals > _signals;
    vector< own< poller > > _pollers;
    own< core::reactor > _reactor;
    message_bus **_buses;

  public:
    shard(u8, message_bus **);
    void init();
    void shutdown();

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

    u16 cpu() const;

  private:
    friend class bootstrap;
    void run();

  private:
    void _push_task_to_inbound_queue(u16 cpu, own< task >);
    void _post_task(own< task >);
    void _handle_exception();
  };

  extern thread_local mut< shard > this_shard;

  inline mut< core::reactor > shard::reactor() {
    return edit(_reactor);
  }

  inline u16 shard::cpu() const {
    return _core_id;
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

mut< core::shard > shard_at(u16);
}
