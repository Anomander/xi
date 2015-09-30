#pragma once

#include "xi/ext/configure.h"
#include "xi/core/executor.h"

namespace xi {
namespace core {

  class kernel;

  class executor_pool : public executor_common< executor_pool > {
    vector< own< executor > > _executors;
    mut< kernel > _kernel;
    unsigned _next_core = 0;

  public:
    executor_pool(mut< kernel > kernel, vector< unsigned > ids)
        : _kernel(kernel) {
      for (auto id : ids) {
        _executors.emplace_back(make< executor >(kernel, id));
      }
    }

    executor_pool(mut< kernel > kernel, size_t count) : _kernel(kernel) {
      for (size_t id = 0; id < count; ++id) {
        _executors.emplace_back(
            make< executor >(kernel, static_cast< unsigned >(id)));
      }
    }

    size_t size() const { return _executors.size(); }

    size_t register_poller(own< poller > poller) {
      auto maybe_local_core = _kernel->local_core_id();
      if (maybe_local_core.is_none()) {
        throw ::std::runtime_error(
            "Unable to register poller from an unmanaged thread.");
      }
      return _kernel->register_poller(maybe_local_core.unwrap(), move(poller));
    }

    void deregister_poller(size_t poller_id) {
      auto maybe_local_core = _kernel->local_core_id();
      if (maybe_local_core.is_none()) {
        throw ::std::runtime_error(
            "Unable to deregister poller from an unmanaged thread.");
      }
      _kernel->deregister_poller(maybe_local_core.unwrap(), poller_id);
    }

    template < class func > void post_on_all(func &&f) {
      for (auto &e : _executors) { e->post(forward< func >(f)); }
    }

    template < class func > void post_on(size_t core, func &&f) {
      executor_for_core(core)->post(forward< func >(f));
    }

    template < class func > void post(func &&f) {
      next_executor()->post(forward< func >(f));
    }

    template < class func > void dispatch(func &&f) {
      next_executor()->dispatch(forward< func >(f));
    }

    mut< executor > executor_for_core(unsigned id) {
      if (id > _executors.size()) {
        throw std::invalid_argument("Executor with id " + to_string(id) +
                                    " is not registered");
      }
      return edit(_executors[id]);
    }

    own< executor > share_executor(unsigned id) {
      return share(executor_for_core(id));
    }

    mut< executor > next_executor() {
      return edit(_executors[(_next_core++) % _executors.size()]);
    }
    own< executor > share_next_executor() { return share(next_executor()); }
    opt< mut< executor > > local_executor() {
      return _kernel->local_core_id().map([this](auto id)
                                              -> opt< mut< executor > > {
        auto it =
            find_if(begin(_executors), end(_executors),
                    [id](own< executor > const &e) { return e->id() == id; });
        if (it == end(_executors)) { return none; }
        return some(edit(*it));
      });
    }
    opt< own< executor > > share_local_executor() {
      auto maybe_local_executor = local_executor();
      return maybe_local_executor.map([](auto exec) { return share(exec); });
    }
  };
}
}
