#include <gtest/gtest.h>

#include "xi/core/Kernel.h"
#include "xi/hw/Hardware.h"
#include "src/test/TestKernel.h"
#include "Util.h"

using namespace xi;
using namespace xi::core;
using namespace xi::test;

TEST(CoreId, LocalCoreId) {
  auto k = make< TestKernel >();
  ASSERT_EQ(kCurrentThread, k->localCoreId().get());

  unsigned id = 1 << 15;
  k->post(kOtherThread, [&]() mutable { id = k->localCoreId().get(); });
  k->runCore(kOtherThread);
  ASSERT_EQ(kOtherThread, id);
}

TEST(CoreId, CoreCountCorrect) {
  auto machine = xi::hw::enumerate();
  unsigned i = 1;
  for (; i <= machine.cpus().size(); ++i) {
    auto k = make< Kernel >();
    k->start(i, 1000);
    ASSERT_EQ(i, k->coreCount());
  }
  auto k = make< Kernel >();
  ASSERT_THROW(k->start(i, 1000), std::invalid_argument);
  ASSERT_THROW(k->start(0U, 1000), std::invalid_argument);
}

struct PollerMock : public test::ObjectTracker, public Poller {
  unsigned poll() noexcept override {
    ++POLLED;
    return 0;
  }

  PollerMock() = default;
  PollerMock(PollerMock const &) = delete;
  PollerMock(PollerMock &&) = delete;

  static void reset() {
    POLLED = 0;
    test::ObjectTracker::reset();
  }

  static size_t POLLED;
};

size_t PollerMock::POLLED = 0UL;

struct PollerTest : public ::testing::Test {
  void SetUp() override { PollerMock::reset(); }
  void TearDown() override { PollerMock::reset(); }
};

TEST_F(PollerTest, PollerOnlyRunsInOwnThread) {
  auto k = make< TestKernel >();
  k->registerPoller(kCurrentThread, make< PollerMock >());
  ASSERT_EQ(0UL, PollerMock::POLLED);
  k->runCore(kOtherThread);
  ASSERT_EQ(0UL, PollerMock::POLLED);
  k->runCore(kCurrentThread);
  ASSERT_EQ(1UL, PollerMock::POLLED);
}

TEST_F(PollerTest, RegisterPoller) {
  auto k = make< TestKernel >();
  k->registerPoller(kCurrentThread, make< PollerMock >());
  ASSERT_EQ(0UL, PollerMock::POLLED);
  k->runCore(kCurrentThread);
  ASSERT_EQ(1UL, PollerMock::POLLED);
}

TEST_F(PollerTest, DeregisterPoller) {
  ASSERT_EQ(0UL, PollerMock::DESTROYED);
  auto k = make< TestKernel >();
  auto id = k->registerPoller(kCurrentThread, make< PollerMock >());
  ASSERT_EQ(0UL, PollerMock::POLLED);
  k->runCore(kCurrentThread);
  ASSERT_EQ(1UL, PollerMock::POLLED);
  k->deregisterPoller(kCurrentThread, id);
  ASSERT_EQ(1UL, PollerMock::DESTROYED);
  PollerMock::reset();
  k->runCore(kCurrentThread);
  ASSERT_EQ(0UL, PollerMock::POLLED);
}

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

TEST(Simple, DispatchOnWrongCoreThrows) {
  auto k = make< TestKernel >();
  ASSERT_THROW(k->dispatch(3, [] {}), std::invalid_argument);
}

TEST(Simple, PostOnWrongCoreThrows) {
  auto k = make< TestKernel >();
  ASSERT_THROW(k->post(3, [] {}), std::invalid_argument);
}

TEST(Exception, InDispatchInOtherThreadShutsDownKernel) {
  auto k = make< TestKernel >();

  ASSERT_NO_THROW(k->dispatch(kOtherThread, [] { throw std::runtime_error("Foo"); }));
  ASSERT_NO_THROW(k->runCore(kOtherThread));
  ASSERT_TRUE(k->isShutDown());
  ASSERT_THROW(k->awaitShutdown(), std::runtime_error);
}

TEST(Exception, InDispatchInCurrentThreadShutsDownKernel) {
  auto k = make< TestKernel >();

  ASSERT_NO_THROW(k->dispatch(kCurrentThread, [] { throw std::runtime_error("Foo"); }));
  ASSERT_TRUE(k->isShutDown());
  ASSERT_THROW(k->awaitShutdown(), std::runtime_error);
}

TEST(Exception, InPostInOtherThreadShutsDownKernel) {
  auto k = make< TestKernel >();

  ASSERT_NO_THROW(k->post(kOtherThread, [] { throw std::runtime_error("Foo"); }));
  ASSERT_NO_THROW(k->runCore(kOtherThread));
  ASSERT_TRUE(k->isShutDown());
  ASSERT_THROW(k->awaitShutdown(), std::runtime_error);
}

TEST(Exception, InPostInCurrentThreadShutsDownKernel) {
  auto k = make< TestKernel >();

  ASSERT_NO_THROW(k->post(kCurrentThread, [] { throw std::runtime_error("Foo"); }));
  ASSERT_NO_THROW(k->runCore(kCurrentThread));
  ASSERT_TRUE(k->isShutDown());
  ASSERT_THROW(k->awaitShutdown(), std::runtime_error);
}

TEST(Exception, InCaseOfConcurrencyTheLastExceptionIsThrown) {
  {
    auto k = make< TestKernel >();

    ASSERT_NO_THROW(k->post(kCurrentThread, [] { throw std::logic_error("Foo"); }));
    ASSERT_NO_THROW(k->post(kOtherThread, [] { throw std::runtime_error("Foo"); }));
    ASSERT_NO_THROW(k->runCore(kCurrentThread));
    ASSERT_NO_THROW(k->runCore(kOtherThread));
    ASSERT_TRUE(k->isShutDown());
    ASSERT_THROW(k->awaitShutdown(), std::runtime_error);
  }
  {
    auto k = make< TestKernel >();

    ASSERT_NO_THROW(k->post(kCurrentThread, [] { throw std::logic_error("Foo"); }));
    ASSERT_NO_THROW(k->post(kOtherThread, [] { throw std::runtime_error("Foo"); }));
    ASSERT_NO_THROW(k->runCore(kOtherThread));
    ASSERT_NO_THROW(k->runCore(kCurrentThread));
    ASSERT_TRUE(k->isShutDown());
    ASSERT_THROW(k->awaitShutdown(), std::logic_error);
  }
}
