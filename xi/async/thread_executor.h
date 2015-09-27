#pragma once

#include "xi/ext/configure.h"
#include "xi/async/executor.h"

namespace xi {
namespace async {

  class thread_executor : public executor {
  public:
    using executor::executor;
    void run(function< void() > f) noexcept override {
      try {
        _thread = thread([ this, f = move(f) ] {
          try {
            this->setup();

            XI_SCOPE(exit) { this->cleanup(); };
            f();
          } catch (exception& e) {
            std::cout << "Exception in executor " << id() << " : " << e.what()
                      << std::endl;
            // satisfy noexcept
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
    thread _thread;
  };
}
}
