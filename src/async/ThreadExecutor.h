#pragma once

#include "ext/Configure.h"
#include "async/Executor.h"

namespace xi {
namespace async {

  class ThreadExecutor : public Executor {
  public:
    using Executor::Executor;
    void run(function< void() > f) noexcept override {
      try {
        _thread = boost::thread([ this, f = move(f) ] {
          try {
            this->setup();

            XI_SCOPE(exit) { this->cleanup(); };
            f();
          } catch (...) {
            std::cout << "Exception in executor " << id() << std::endl;
            // satisfy noexcept
          }
        });
      } catch (...) {
        // satisfy noexcept
      }
    }

    void join() override { _thread.join(); }

  public:
    boost::thread _thread;
  };

}
}
