#pragma once

#include "xi/ext/configure.h"
#include "xi/core/generic_resumable.h"
#include "xi/core/runtime.h"

namespace xi {
namespace core {
  namespace v2 {
    template < class F, class... Args >
    class generic_resumable_builder : public resumable_builder {
      tuple< Args... > _args;

    public:
      generic_resumable_builder(Args&&... args)
          : _args(forward< Args >(args)...) {
      }

      own< resumable > build() final override {
        return make< F >(piecewise_construct, move(_args));
      };
    };

    template < class F,
               class... Args,
               XI_REQUIRE_DECL(is_base_of< resumable, F >) >
    void spawn(Args&&... args) {
      runtime.spawn(make< generic_resumable_builder< F, Args... > >(
          forward< Args >(args)...));
    }

    template < class F,
               XI_UNLESS_DECL(is_base_of< resumable, F >),
               XI_REQUIRE_DECL(is_callable< F, void() >) >
    void spawn(F&& f) {
      struct delegate_resumable : public generic_resumable {
        delegate_resumable(F&& f) : _f(forward< F >(f)) {
        }

      private:
        void call() {
          _f();
        }
        F _f;
      };

      runtime.spawn(make< generic_resumable_builder< delegate_resumable, F > >(
          forward< F >(f)));
    }
  }

  template < class F,
             class... Args,
             XI_REQUIRE_DECL(is_base_of< resumable, F >) >
  void spawn(Args&&... args) {
    auto& coordinator = runtime.coordinator();
    auto maker        = [](Args&&... args) {
      runtime.local_worker().spawn_resumable< F >(forward< Args >(args)...);
    };
    coordinator.schedule(make_lambda_blocking_resumable(
        xi::bind(maker, forward< Args >(args)...)));
  }

  template < class F,
             XI_UNLESS_DECL(is_base_of< resumable, F >),
             XI_REQUIRE_DECL(is_callable< F, void() >) >
  void spawn(F&& f) {
    struct delegate_resumable : public generic_resumable {
      delegate_resumable(F&& f) : _f(forward< F >(f)) {
      }

    private:
      void call() {
        _f();
      }
      F _f;
    };

    auto& coordinator = runtime.coordinator();
    auto maker = [f = move(f)]() mutable {
      runtime.local_worker().spawn_resumable< delegate_resumable >(move(f));
    };
    coordinator.schedule(make_lambda_blocking_resumable(move(maker)));
  }
}
}
