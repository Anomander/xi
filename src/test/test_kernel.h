#include "xi/core/kernel.h"

namespace xi {
namespace test {

  enum : unsigned { kCurrentThread = 0, kOtherThread = 1 };

  class test_kernel : public core::kernel {
    bool _shutdown_initiated = false;

  public:
    test_kernel() : kernel() {
      start(2, 1000);
      startup(0);
    }

    ~test_kernel() { cleanup(); }

  public:
    void run_core(unsigned id) {
      cleanup();
      startup(id);
      poll_core(id);
      cleanup();
      startup(kCurrentThread);
    }

    bool is_shut_down() const { return _shutdown_initiated; }

  private:
    void initiate_shutdown() override {
      _shutdown_initiated = true;
      kernel::initiate_shutdown();
    };
  };
}
}
