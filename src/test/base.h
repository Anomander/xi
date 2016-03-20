#include "src/test/mock_kernel.h"
#include "src/test/util.h"

#include <gtest/gtest.h>

namespace xi {
  namespace test {

    struct base : public ::testing::Test {
      own< test::mock_kernel > _kernel;

    public:
      void SetUp() override {
        _kernel = make< test::mock_kernel >();
      }
      void TearDown() override {
      }

    protected:
      void poll_core(u16 id = kCurrentThread) {
        _kernel->run_core(id);
      }
    };

  }
}
