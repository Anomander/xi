#include "src/test/mock_kernel.h"
#include "src/test/util.h"

#include <gtest/gtest.h>

namespace xi {
  namespace test {

    struct base : public ::testing::Test {
      own< test::mock_kernel > _kernel;

    public:
      virtual void set_up() {
        _kernel = make< test::mock_kernel >();
      }
      virtual void tear_down() {
      }

    protected:
      void poll_core(u16 id = kCurrentThread) {
        _kernel->run_core(id);
      }

    private:
      void SetUp() final override {
        set_up();
      }
      void TearDown() final override {
        tear_down();
      }

    };

  }
}
