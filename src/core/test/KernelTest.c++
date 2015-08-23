#include <gtest/gtest.h>

#include "core/Kernel.h"

using namespace xi;
using namespace xi::core;

enum : unsigned { kCurrentThread = 0, kOtherThread = 1};

class TestKernel : public Kernel {
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
};

// The local thread is considered managed if Kernel::startup was run in it.
TEST(Simple, DispatchRunsInlineWhenInSameThread) {
  auto k = make< TestKernel >();

  int i = 0;
  k->dispatch(kCurrentThread, [&i] { i = 42; });

  ASSERT_EQ(42, i);
}

TEST(Simple, DispatchRunsAsyncWhenInDifferentThread) {
  auto k = make< TestKernel >();

  int i = 0;
  k->dispatch(kOtherThread, [&i] { i = 42; });

  ASSERT_EQ(0, i);

  k->runCore(kOtherThread);

  ASSERT_EQ(42, i);
}

TEST(Simple, PostAlwaysRunsAsync) {
  auto k = make< TestKernel >();

  int i = 0;
  k->post(kCurrentThread, [&i] { i = 42; });

  ASSERT_EQ(0, i);

  k->runCore(kCurrentThread);

  ASSERT_EQ(42, i);
}
