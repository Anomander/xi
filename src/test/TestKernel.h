#include "xi/core/Kernel.h"

namespace xi {
namespace test {

  enum : unsigned { kCurrentThread = 0, kOtherThread = 1 };

  class TestKernel : public core::Kernel {
    bool _shutdownInitiated = false;

  public:
    TestKernel() : Kernel() {
      start(2, 1000);
      startup(0);
    }

    ~TestKernel() { cleanup(); }

  public:
    void runCore(unsigned id) {
      cleanup();
      startup(id);
      pollCore(id);
      cleanup();
      startup(kCurrentThread);
    }

    bool isShutDown() const { return _shutdownInitiated; }

  private:
    void initiateShutdown() override {
      _shutdownInitiated = true;
      Kernel::initiateShutdown();
    };
  };
}
}
