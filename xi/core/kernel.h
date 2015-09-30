#pragma once

#include "xi/async/context.h"
#include "xi/core/task_queue.h"
#include "xi/util/spin_lock.h"
#include "xi/hw/hardware.h"

namespace xi {
namespace core {

  struct alignas(64) poller : public virtual ownership::unique {
    virtual ~poller() = default;
    virtual unsigned poll() noexcept = 0;
  };

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
    vector< vector< own< task_queue > > > _queues;
    vector< vector< own< poller > > > _pollers;
    vector< inbound_tasks > _inbound_task_queues;
    exception_filter_type _exception_filter;
    exception_ptr _exit_exception;
    spin_lock _exception_lock;

  public:
    kernel();
    explicit kernel(hw::machine);

    virtual ~kernel();

    virtual void start(unsigned count, unsigned per_core_queue_size);
    virtual void initiate_shutdown();
    virtual void await_shutdown();
    virtual void exception_filter(exception_filter_type);
    virtual void handle_exception(exception_ptr);

    size_t register_poller(unsigned core, own< poller > poller);
    void deregister_poller(unsigned core, size_t poller_id);

    template < class func > void dispatch(unsigned core, func &&f);
    template < class func > void post(unsigned core, func &&f);

    static opt< unsigned > local_core_id();
    unsigned core_count();

  protected:
    void run_on_core(unsigned id);
    void poll_core(unsigned id);
    void startup(unsigned id);
    void cleanup();

    hw::machine &machine() { return _machine; }

  private:
    template < class func >
    void _push_task_to_inbound_queue(unsigned core, func &&f);
    template < class func >
    void _push_task_to_inbound_queue(unsigned core, func &&f, meta::true_type);
    template < class func >
    void _push_task_to_inbound_queue(unsigned core, func &&f, meta::false_type);
  };

  template < class func > void kernel::dispatch(unsigned core, func &&f) {
    auto maybe_local_core = local_core_id();
    if (maybe_local_core == some(core)) {
      try {
        f();
      } catch (...) { handle_exception(current_exception()); }
    } else { post(core, forward< func >(f)); }
  }

  template < class func > void kernel::post(unsigned core, func &&f) {
    if (core > _queues.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    try {
      auto maybe_id = local_core_id();
      if (maybe_id.is_none()) {
        // local thread is not managed by the kernel, so
        // we must use a common input queue
        _push_task_to_inbound_queue(core, forward< func >(f));
      } else { _queues[core][maybe_id.unwrap()]->submit(forward< func >(f)); }
    } catch (...) { handle_exception(current_exception()); }
  }

  template < class func >
  void kernel::_push_task_to_inbound_queue(unsigned core, func &&f) {
    auto lock = make_lock(_inbound_task_queues[core].lock);
    _push_task_to_inbound_queue(core, forward< func >(f),
                                is_base_of< task, decay_t< func > >());
  }

  template < class func >
  void kernel::_push_task_to_inbound_queue(unsigned core, func &&f,
                                           meta::true_type) {
    std::cout << "New inbound task" << std::endl;
    _inbound_task_queues[core].tasks.push(make_unique_copy(forward< func >(f)));
  }

  template < class func >
  void kernel::_push_task_to_inbound_queue(unsigned core, func &&f,
                                           meta::false_type) {
    _inbound_task_queues[core].tasks.push(
        make_unique_copy(make_task(forward< func >(f))));
  }
}
}
