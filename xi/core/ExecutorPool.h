#pragma once

#include "xi/ext/Configure.h"
#include "xi/core/Executor.h"

namespace xi {
namespace core {

  class Kernel;

  class ExecutorPool : public virtual ownership::StdShared {
    vector< Executor > _executors;
    mut< Kernel > _kernel;
    unsigned _nextCore = 0;

  public:
    ExecutorPool(mut< Kernel > kernel, vector< unsigned > ids) : _kernel(kernel) {
      for (auto id : ids) {
        _executors.emplace_back(kernel, id);
      }
    }

    ExecutorPool(mut< Kernel > kernel, size_t count) : _kernel(kernel) {
      for (size_t id = 0; id < count; ++id) {
        _executors.emplace_back(kernel, static_cast< unsigned >(id));
      }
    }

    size_t size() const { return _executors.size(); }

    size_t registerPoller(own<Poller> poller) {
      auto maybeLocalCore = _kernel->localCoreId();
      if (! maybeLocalCore) {
        throw ::std::runtime_error("Unable to register poller from an unmanaged thread.");
      }
      return _kernel->registerPoller(maybeLocalCore.get(), move(poller));
    }

    void deregisterPoller(size_t pollerId) {
      auto maybeLocalCore = _kernel->localCoreId();
      if (! maybeLocalCore) {
        throw ::std::runtime_error("Unable to deregister poller from an unmanaged thread.");
      }
      _kernel->deregisterPoller(maybeLocalCore.get(), pollerId);
    }

    template < class Func >
    void postOnAll(Func &&f) {
      for (auto &e : _executors) {
        e.post(forward< Func >(f));
      }
    }

    template < class Func >
    void postOn(size_t core, Func &&f) {
      if (core > _executors.size()) {
        throw std::invalid_argument("Core not registered");
      }
      _executors[core].post(forward< Func >(f));
    }

    template < class Func >
    void post(Func &&f) {
      nextExecutor()->post(forward< Func >(f));
    }

    template < class Func >
    void dispatch(Func &&f) {
      nextExecutor()->dispatch(forward< Func >(f));
    }

    // Returns a function which when called will post the
    // wrapped function into the executor pool.
    // Use this when something wants a runnable argument,
    // but you want to run the execution asynchronously.
    template<class Func>
    auto wrap(Func && f){
      return [f=forward<Func>(f), this] (auto ... args) { this->post(::std::bind(f, forward<decltype(args)...>(args)...)); };
    }

  protected:
    mut< Executor > nextExecutor() { return edit(_executors[(_nextCore++) % _executors.size()]); }
  };
}
}
