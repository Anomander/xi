#include "xi/core/kernel.h"

namespace xi {
namespace test {

  enum : unsigned { kCurrentThread = 0, kOtherThread = 1 };

  class mock_kernel : public core::kernel {
    bool _shutdown_initiated = false;

  public:
  mock_kernel() : kernel() {
      start(2, 1000);
      core::this_shard = nullptr;
      startup(kOtherThread);

      // current thread must be last
      core::this_shard = nullptr;
      startup(kCurrentThread);
      core::this_shard = mut_shard(kCurrentThread);
    }

    ~mock_kernel() {
      core::this_shard = mut_shard(kOtherThread);
      cleanup(kOtherThread);
      core::this_shard = mut_shard(kCurrentThread);
      cleanup(kCurrentThread);
    }

  public:
    void run_core(unsigned id) {
      core::this_shard = mut_shard(id);
      poll_core(id);
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
