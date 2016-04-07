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

  class kernel;

  class alignas(64) shard final {
    u16 _core_id = -1;

    struct {
      lockfree::queue< task * > tasks{128};
    } _inbound;

    task_queue _task_queue;
    vector< own< poller > > _pollers;
    mut< core::reactor > _reactor;
    mut< kernel > _kernel;

  public:
    shard(mut< kernel >);
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

  template < class F >
  void shard::post(F &&func) {
    if (nullptr == this_shard || this != this_shard) {
      // local thread is not managed by the kernel
      // or is a remote thread
      // , so we must use a common input queue
      _push_task_to_inbound_queue(forward< F >(func));
    } else {
      _task_queue.submit(forward< F >(func));
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
    _push_task_to_inbound_queue(forward< F >(func),
                                is_base_of< task, decay_t< F > >());
  }

  template < class F >
  void shard::_push_task_to_inbound_queue(F &&func, meta::true_type) {
    _inbound.tasks.push(make_unique_copy(forward< F >(func)).release());
  }

  template < class F >
  void shard::_push_task_to_inbound_queue(F &&func, meta::false_type) {
    _inbound.tasks.push(
        make_unique_copy(make_task(forward< F >(func))).release());
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
