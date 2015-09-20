#pragma once

#include "xi/ext/Configure.h"
#include "xi/core/Executor.h"

namespace xi {
namespace async {

  template < class T >
  class Async {
    opt< own< core::Executor > > _executor;

  protected:
    virtual ~Async() = default;

  public:
    virtual void attachExecutor(own< core::Executor > e) { _executor = move(e); }
    virtual void detachExecutor() {
      if (_executor) {
        release(move(_executor.get()));
      }
    }

    template < class Func >
    void defer(Func&& f) {
      if (!_executor) {
        throw std::logic_error("Invalid async object state.");
      }
      _executor.get()->post(forward< Func >(f));
    }

    template < class Func >
    decltype(auto) makeDeferred(Func&& f) {
      if (!_executor) {
        throw std::logic_error("Invalid async object state.");
      }
      return _executor.get()->wrap(forward< Func >(f));
    }

  private:
    friend T;
    mut< core::Executor > executor() {
      assert(_executor);
      return edit(_executor.get());
    }
    own< core::Executor > shareExecutor() { return share(executor()); }
  };
}
}
