#pragma once

#include "xi/ext/Configure.h"
#include "xi/core/Executor.h"

namespace xi {
namespace core {

  class Kernel;

  class ExecutorPool : public ExecutorCommon< ExecutorPool > {
    vector< own< Executor > > _executors;
    mut< Kernel > _kernel;
    unsigned _nextCore = 0;

  public:
    ExecutorPool(mut< Kernel > kernel, vector< unsigned > ids) : _kernel(kernel) {
      for (auto id : ids) {
        _executors.emplace_back(make< Executor >(kernel, id));
      }
    }

    ExecutorPool(mut< Kernel > kernel, size_t count) : _kernel(kernel) {
      for (size_t id = 0; id < count; ++id) {
        _executors.emplace_back(make< Executor >(kernel, static_cast< unsigned >(id)));
      }
    }

    size_t size() const { return _executors.size(); }

    size_t registerPoller(own< Poller > poller) {
      auto maybeLocalCore = _kernel->localCoreId();
      if (!maybeLocalCore) {
        throw ::std::runtime_error("Unable to register poller from an unmanaged thread.");
      }
      return _kernel->registerPoller(maybeLocalCore.get(), move(poller));
    }

    void deregisterPoller(size_t pollerId) {
      auto maybeLocalCore = _kernel->localCoreId();
      if (!maybeLocalCore) {
        throw ::std::runtime_error("Unable to deregister poller from an unmanaged thread.");
      }
      _kernel->deregisterPoller(maybeLocalCore.get(), pollerId);
    }

    template < class Func >
    void postOnAll(Func &&f) {
      for (auto &e : _executors) {
        e->post(forward< Func >(f));
      }
    }

    template < class Func >
    void postOn(size_t core, Func &&f) {
      executor(core)->post(forward< Func >(f));
    }

    template < class Func >
    void post(Func &&f) {
      nextExecutor()->post(forward< Func >(f));
    }

    template < class Func >
    void dispatch(Func &&f) {
      nextExecutor()->dispatch(forward< Func >(f));
    }

    mut< Executor > executor(unsigned id) {
      if (id > _executors.size()) {
        throw std::invalid_argument("Executor with id " + to_string(id) + " is not registered");
      }
      return edit(_executors[id]);
    }

    own< Executor > shareExecutor(unsigned id) { return share(executor(id)); }

    mut< Executor > nextExecutor() { return edit(_executors[(_nextCore++) % _executors.size()]); }
    own< Executor > shareNextExecutor() { return share(nextExecutor()); }
    opt< mut< Executor > > localExecutor() {
      auto maybeLocalCore = _kernel->localCoreId();
      if (!maybeLocalCore) {
        return none;
      }

      auto localCoreId = maybeLocalCore.get();
      auto it = find_if(begin(_executors), end(_executors),
                        [localCoreId](own< Executor > const &e) { return e->id() == localCoreId; });
      if (it == end(_executors)) {
        return none;
      }
      return edit(*it);
    }
    opt< own< Executor > > shareLocalExecutor() {
      auto maybeLocalExecutor = localExecutor();
      if (!maybeLocalExecutor) {
        return none;
      }
      return share(maybeLocalExecutor.get());
    }
  };
}
}
