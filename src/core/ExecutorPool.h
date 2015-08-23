#pragma once

#include "ext/Configure.h"
#include "core/Executor.h"

namespace xi {
namespace core {

  class Kernel;

  class ExecutorPool : public virtual ownership::StdShared {
    vector< Executor > _executors;
    unsigned _nextCore = 0;

  public:
    ExecutorPool(mut< Kernel > kernel, vector< unsigned > ids) {
      for (auto id : ids) {
        _executors.emplace_back(kernel, id);
      }
    }

    ExecutorPool(mut< Kernel > kernel, size_t count) {
      for (size_t id = 0; id < count; ++id) {
        _executors.emplace_back(kernel, static_cast< unsigned >(id));
      }
    }

    template < class Func >
    void postOnAll(Func && f) {
      for (auto &e : _executors) {
        e.post(forward<Func>(f));
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
    void dispatchOn(size_t core, Func &&f) {
      if (core > _executors.size()) {
        throw std::invalid_argument("Core not registered");
      }
      _executors[core].dispatch(forward< Func >(f));
    }

    template < class Func >
    void post(Func &&f) {
      nextExecutor()->post(forward< Func >(f));
    }

    template < class Func >
    void dispatch(Func &&f) {
      nextExecutor()->dispatch(forward< Func >(f));
    }

  protected:
    mut< Executor > nextExecutor() { return edit(_executors[(_nextCore++) % _executors.size()]); }
  };
}
}
