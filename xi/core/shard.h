#pragma once

#include "xi/core/reactor.h"
#include "xi/core/task_queue.h"
#include "xi/util/spin_lock.h"

namespace xi {
namespace core {

  struct shard_stats {
    usize iterations = 0;
    nanoseconds total_spent = 0ns;
    nanoseconds total_queues = 0ns;
    nanoseconds total_pollers = 0ns;
    nanoseconds total_inbound = 0ns;
  };

  struct alignas(64) poller : public virtual ownership::unique {
    virtual ~poller()                = default;
    virtual unsigned poll() noexcept = 0;
  };

  class kernel;

  class alignas(64) shard final {
    u16 _core_id = -1;

    using queues_t = vector< vector< own< task_queue > > >;
    struct {
      queue< unique_ptr< task > > tasks;
      spin_lock lock;
    } _inbound;

    queues_t &_queues;
    vector< own< poller > > _pollers;
    mut< kernel > _kernel;
    mut< core::reactor > _reactor;
    shard_stats _stats;

  public:
    shard(mut< kernel > k, u16 core, queues_t &qs);
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
    ref< shard_stats> stats();

  private:
    template < class F >
    void _push_task_to_inbound_queue(F &&);
    template < class F >
    void _push_task_to_inbound_queue(F &&func, meta::true_type);
    template < class F >
    void _push_task_to_inbound_queue(F &&func, meta::false_type);

    void _handle_exception();
  };

  extern thread_local mut< shard > this_shard;

  inline mut< core::reactor > shard::reactor() {
    return _reactor;
  }

  inline ref< shard_stats > shard::stats() {
    return _stats;
  }

  template < class F >
  void shard::post(F &&func) {
    if (nullptr == this_shard) {
      // local thread is not managed by the kernel, so
      // we must use a common input queue
      _push_task_to_inbound_queue(forward< F >(func));
    } else {
      _queues[_core_id][this_shard->_core_id]->submit(forward< F >(func));
    }
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

  template < class F >
  void shard::_push_task_to_inbound_queue(F &&func) {
    auto lock = make_lock(_inbound.lock);
    _push_task_to_inbound_queue(forward< F >(func),
                                is_base_of< task, decay_t< F > >());
  }

  template < class F >
  void shard::_push_task_to_inbound_queue(F &&func, meta::true_type) {
    _inbound.tasks.push(make_unique_copy(forward< F >(func)));
  }

  template < class F >
  void shard::_push_task_to_inbound_queue(F &&func, meta::false_type) {
    _inbound.tasks.push(make_unique_copy(make_task(forward< F >(func))));
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
