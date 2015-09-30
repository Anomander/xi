#pragma once

#include "xi/ext/configure.h"
#include "xi/core/executor.h"

namespace xi {
namespace async {

  template < class T > class async {
    own< core::executor > _executor;

  protected:
    virtual ~async() = default;

  public:
    virtual void attach_executor(own< core::executor > e) {
      _executor = move(e);
    }
    virtual void detach_executor() {
      release(_executor);
    }

    template < class func > void defer(func &&f) {
      if (! is_valid(_executor)) { throw std::logic_error("Invalid async object state."); }
      _executor->post(forward< func >(f));
    }

    template < class func > decltype(auto) make_deferred(func &&f) {
      if (! is_valid(_executor)) { throw std::logic_error("Invalid async object state."); }
      return _executor->wrap(forward< func >(f));
    }

  private:
    friend T;
    mut< core::executor > executor() {
      assert(is_valid(_executor));
      return edit(_executor);
    }
    own< core::executor > share_executor() { return share(executor()); }
  };
}
}
