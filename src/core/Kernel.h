#pragma once

#include "async/Context.h"
#include "core/TaskQueue.h"
#include "util/SpinLock.h"
#include "hw/Hardware.h"

namespace xi {
namespace core {

  class Kernel : public virtual ownership::Unique {
    hw::Machine _machine;
    struct CoreDescriptor {
      hw::Cpu const &cpu;
      unsigned id;
    };
    struct InboundTasks {
      queue< unique_ptr< Task > > tasks;
      SpinLock lock;
    };
    vector< vector< own< TaskQueue > > > _queues;
    vector< InboundTasks > _inboundTaskQueues;

  public:
    Kernel();
    Kernel(hw::Machine);

    virtual ~Kernel();

    virtual void start(unsigned count, unsigned perCoreQueueSize);
    virtual void initiateShutdown();
    virtual void awaitShutdown() {}

    template < class Func >
    void dispatch(unsigned core, Func &&f) {
      if (core == localCoreId()) {
        f();
      } else {
        post(core, forward< Func >(f));
      }
    }

    template < class Func >
    void post(unsigned core, Func &&f) {
      // static size_t *N = nullptr;
      // if (!N) {
      //   N = new size_t[_queues.size() * _queues.size()];
      //   for (unsigned i = 0; i < _queues.size() * _queues.size(); ++i) {
      //     N[i] = 0;
      //   }
      // }

      if (core > _queues.size()) {
        throw std::invalid_argument("Target core not registered");
      }
      auto *desc = async::tryLocal< CoreDescriptor >();
      if (!desc) {
        // local thread is not managed by the kernel, so
        // we must use a common input queue
        _pushTaskToInboundQueue(core, forward< Func >(f));
      } else {
        // auto &n = N[core * _queues.size() + desc->id];
        // ++n;
        // if (n % 10000 == 0) {
        //   std::cout << "Posts on queue " << core << " from queue " << desc->id << " : " << n << std::endl;
        // }
        _queues[core][desc->id]->submit(forward< Func >(f));
      }
    }

    static unsigned localCoreId();
    unsigned coreCount();

  protected:
    void runOnCore(unsigned id);
    void pollCore(unsigned id);
    void startup(unsigned id);
    void cleanup();

    hw::Machine &machine() { return _machine; }

  private:
    template < class Func >
    void _pushTaskToInboundQueue(unsigned core, Func &&f) {
      auto lock = makeLock(_inboundTaskQueues[core].lock);
      _pushTaskToInboundQueue(core, forward< Func >(f), is_base_of< Task, decay_t< Func > >());
    }
    template < class Func >
    void _pushTaskToInboundQueue(unsigned core, Func &&f, meta::TrueType) {
      std::cout << "New inbound task" << std::endl;
      _inboundTaskQueues[core].tasks.push(makeUniqueCopy(forward< Func >(f)));
    }
    template < class Func >
    void _pushTaskToInboundQueue(unsigned core, Func &&f, meta::FalseType) {
      _inboundTaskQueues[core].tasks.push(makeUniqueCopy(makeTask(forward< Func >(f))));
    }
  };
}
}
