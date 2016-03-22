#pragma once

#include "xi/async/future.h"
#include "xi/core/shard.h"
#include "xi/core/task_queue.h"
#include "xi/hw/hardware.h"
#include "xi/util/spin_lock.h"

namespace xi {
namespace core {

  class kernel : public virtual ownership::unique {
  public:
    /// returns true if exception should be swalowed and the thread can continue
    /// operating, false otherwise.
    using exception_filter_type = function< bool(exception_ptr) >;

  private:
    hw::machine _machine;
    struct alignas(64) inbound_tasks {
      queue< unique_ptr< task > > tasks;
      spin_lock lock;
    };

    vector< mut< shard > > _shards;
    vector< vector< own< task_queue > > > _queues;
    exception_filter_type _exception_filter;
    exception_ptr _exit_exception;
    spin_lock _exception_lock;

  public:
    kernel();
    explicit kernel(hw::machine);

    virtual ~kernel();

    virtual async::future<> start(unsigned count, unsigned per_core_queue_size);
    virtual void initiate_shutdown();
    virtual void await_shutdown();
    virtual void exception_filter(exception_filter_type);
    virtual void handle_exception(exception_ptr);

    usize register_poller(u16 core, own< poller > poller);
    void deregister_poller(u16 core, usize poller_id);

    template < class func >
    void dispatch(unsigned core, func &&f);
    template < class func >
    void post(unsigned core, func &&f);

    unsigned core_count();

    mut< shard > mut_shard(u16 core);

  protected:
    virtual void run_on_core(unsigned id);
    virtual void poll_core(unsigned id);
    virtual void startup(u16 id);
    virtual void cleanup(u16 id);

    hw::machine &machine() {
      return _machine;
    }
  };

  inline mut< shard > kernel::mut_shard(u16 core) {
    if (core >= _shards.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    return _shards[core];
  }

  template < class func >
  void kernel::dispatch(unsigned core, func &&f) {
    if (core >= _queues.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    _shards[core]->dispatch(forward< func >(f));
  }

  template < class func >
  void kernel::post(unsigned core, func &&f) {
    if (core >= _queues.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    _shards[core]->post(forward< func >(f));
  }
}
}
