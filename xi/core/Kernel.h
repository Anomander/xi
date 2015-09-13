#pragma once

#include "xi/async/Context.h"
#include "xi/core/TaskQueue.h"
#include "xi/util/SpinLock.h"
#include "xi/hw/Hardware.h"

namespace xi {
namespace core {

  using async::makeTask;

  struct alignas(64) Poller : public virtual ownership::Unique {
    virtual ~Poller() = default;
    virtual unsigned poll() noexcept = 0;
  };

  class Kernel : public virtual ownership::Unique {
  public:
    /// Returns true if exception should be swalowed and the thread can continue
    /// operating, false otherwise.
    using ExceptionFilter = function< bool(exception_ptr) >;

  private:
    hw::Machine _machine;
    struct alignas(64) InboundTasks {
      queue< unique_ptr< Task > > tasks;
      SpinLock lock;
    };
    vector< vector< own< TaskQueue > > > _queues;
    vector< vector< own< Poller > > > _pollers;
    vector< InboundTasks > _inboundTaskQueues;
    ExceptionFilter _exceptionFilter;
    exception_ptr _exitException;
    SpinLock _exceptionLock;

  public:
    Kernel();
    explicit Kernel(hw::Machine);

    virtual ~Kernel();

    virtual void start(unsigned count, unsigned perCoreQueueSize);
    virtual void initiateShutdown();
    virtual void awaitShutdown();
    virtual void exceptionFilter(ExceptionFilter);
    virtual void handleException(exception_ptr);

    size_t registerPoller(unsigned core, own< Poller > poller);
    void deregisterPoller(unsigned core, size_t pollerId);

    template < class Func >
    void dispatch(unsigned core, Func &&f);
    template < class Func >
    void post(unsigned core, Func &&f);

    static opt< unsigned > localCoreId();
    unsigned coreCount();

  protected:
    void runOnCore(unsigned id);
    void pollCore(unsigned id);
    void startup(unsigned id);
    void cleanup();

    hw::Machine &machine() { return _machine; }

  private:
    template < class Func >
    void _pushTaskToInboundQueue(unsigned core, Func &&f);
    template < class Func >
    void _pushTaskToInboundQueue(unsigned core, Func &&f, meta::TrueType);
    template < class Func >
    void _pushTaskToInboundQueue(unsigned core, Func &&f, meta::FalseType);
  };

  template < class Func >
  void Kernel::dispatch(unsigned core, Func &&f) {
    auto maybeLocalCore = localCoreId();
    if (maybeLocalCore && core == maybeLocalCore.get()) {
      try {
        f();
      } catch (...) {
        handleException(current_exception());
      }
    } else {
      post(core, forward< Func >(f));
    }
  }

  template < class Func >
  void Kernel::post(unsigned core, Func &&f) {
    if (core > _queues.size()) {
      throw std::invalid_argument("Target core not registered");
    }
    try {
      auto maybeId = localCoreId();
      if (!maybeId) {
        // local thread is not managed by the kernel, so
        // we must use a common input queue
        _pushTaskToInboundQueue(core, forward< Func >(f));
      } else {
        _queues[core][maybeId.get()]->submit(forward< Func >(f));
      }
    } catch (...) {
      handleException(current_exception());
    }
  }

  template < class Func >
  void Kernel::_pushTaskToInboundQueue(unsigned core, Func &&f) {
    auto lock = makeLock(_inboundTaskQueues[core].lock);
    _pushTaskToInboundQueue(core, forward< Func >(f), is_base_of< Task, decay_t< Func > >());
  }

  template < class Func >
  void Kernel::_pushTaskToInboundQueue(unsigned core, Func &&f, meta::TrueType) {
    std::cout << "New inbound task" << std::endl;
    _inboundTaskQueues[core].tasks.push(makeUniqueCopy(forward< Func >(f)));
  }

  template < class Func >
  void Kernel::_pushTaskToInboundQueue(unsigned core, Func &&f, meta::FalseType) {
    _inboundTaskQueues[core].tasks.push(makeUniqueCopy(makeTask(forward< Func >(f))));
  }
}
}
