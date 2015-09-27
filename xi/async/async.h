#pragma once

#include "xi/ext/configure.h"
#include "xi/core/executor.h"

namespace xi {
namespace async {

  template < class T > class async {
    opt< own< core::executor > > _executor;

  protected:
    virtual ~async() = default;

  public:
    virtual void attach_executor(own< core::executor > e) {
      _executor = move(e);
    }
    virtual void detach_executor() {
      if (_executor) { release(move(_executor.get())); }
    }

    template < class func > void defer(func &&f) {
      if (!_executor) { throw std::logic_error("Invalid async object state."); }
      _executor.get()->post(forward< func >(f));
    }

    template < class func > decltype(auto) make_deferred(func &&f) {
      if (!_executor) { throw std::logic_error("Invalid async object state."); }
      return _executor.get()->wrap(forward< func >(f));
    }

  private:
    friend T;
    mut< core::executor > executor() {
      assert(_executor);
      return edit(_executor.get());
    }
    own< core::executor > share_executor() { return share(executor()); }
  };
}
}
