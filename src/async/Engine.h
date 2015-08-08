#pragma once

#include "async/Context.h"
#include "async/Async.h"
#include "async/ExecutorGroup.h"
#include "async/Latch.h"

namespace xi {
namespace async {

  template < class E, class R >
  class Engine {
  public:
    auto setup() {
      auto coreCount = 8UL;                 // get from config
      auto perCoreQueueSize = 100 * 1024UL; // get from config
      _executors = make< ExecutorGroup< E > >(coreCount, perCoreQueueSize);
      return makeReadyFuture();
    }

    auto run() {
      auto coreCount = 8UL; // get from config
      Latch latch {coreCount};
      auto future = latch.await();
      _executors->run([&]() mutable {
        auto &exec = local< Executor >();

        std::cout << "Executor " << exec.id() << std::endl;
        _executors->executeOn((exec.id() + 1) % coreCount,
                              [&] { std::cout << "Hello from exec " << exec.id() << std::endl; });
        for (;;) {
          // reactor.poll();
          exec.processQueues();
        }
        latch.countDown();
      });
      return future;
    }

    template < class Func >
    void runOn(size_t core, Func &&f) {
      _executors->executeOn(core, forward< Func >(f));
    }

    template < class Func >
    void runOnNext(Func &&f) {
      _executors->executeOnNext(forward< Func >(f));
    }

  private:
    Engine() = default;
    template < class >
    friend class LocalStorage;

  private:
    own< ExecutorGroup< E > > _executors;
  };

  /// Override to always return the same Engine object.
  template < class E, class R >
  struct LocalStorage< Engine< E, R > > {
    static Engine< E, R > &get() {
      static Engine< E, R > OBJ;
      return OBJ;
    }
  };
}
}
