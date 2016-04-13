#include <gtest/gtest.h>

#include "xi/core/kernel.h"
#include "xi/hw/hardware.h"
#include "src/test/mock_kernel.h"
#include "src/test/util.h"

using namespace xi;
using namespace xi::core;
using namespace xi::test;

TEST(core_id, core_count_correct) {
  auto machine = xi::hw::enumerate();
  unsigned i   = 1;
  for (; i <= machine.cpus().size(); ++i) {
    auto k = make< kernel >();
    k->start(i, 1000);
    EXPECT_EQ(i, k->core_count());
  }
  auto k  = make< kernel >();
  auto f1 = k->start(i, 1000);
  EXPECT_TRUE(f1.is_exception());
  auto f2 = k->start(i, 1000);
  EXPECT_TRUE(f2.is_exception());
}

struct poller_mock : public test::object_tracker, public poller {
  unsigned poll() noexcept override {
    ++POLLED;
    return 0;
  }

  poller_mock()                    = default;
  poller_mock(poller_mock const &) = delete;
  poller_mock(poller_mock &&)      = delete;

  static void reset() {
    POLLED = 0;
    test::object_tracker::reset();
  }

  static size_t POLLED;
};

size_t poller_mock::POLLED = 0UL;

struct poller_test : public ::testing::Test {
  void SetUp() override {
    poller_mock::reset();
  }
  void TearDown() override {
    poller_mock::reset();
  }
};

TEST_F(poller_test, poller_only_runs_in_own_thread) {
  auto k = make< mock_kernel >();
  k->register_poller(kCurrentThread, make< poller_mock >());
  EXPECT_EQ(0UL, poller_mock::POLLED);
  k->run_core(kOtherThread);
  EXPECT_EQ(0UL, poller_mock::POLLED);
  k->run_core(kCurrentThread);
  EXPECT_EQ(1UL, poller_mock::POLLED);
}

TEST_F(poller_test, register_poller) {
  auto k = make< mock_kernel >();
  k->register_poller(kCurrentThread, make< poller_mock >());
  EXPECT_EQ(0UL, poller_mock::POLLED);
  k->run_core(kCurrentThread);
  EXPECT_EQ(1UL, poller_mock::POLLED);
}

TEST_F(poller_test, deregister_poller) {
  EXPECT_EQ(0UL, poller_mock::DESTROYED);
  std::cout << this_shard << std::endl;

  auto k = make< mock_kernel >();
  std::cout << this_shard << std::endl;

  auto id = k->register_poller(kCurrentThread, make< poller_mock >());
  std::cout << this_shard << std::endl;

  EXPECT_EQ(0UL, poller_mock::POLLED);
  k->run_core(kCurrentThread);
  EXPECT_EQ(1UL, poller_mock::POLLED);
  k->deregister_poller(kCurrentThread, id);
  EXPECT_EQ(1UL, poller_mock::DESTROYED);
  poller_mock::reset();
  k->run_core(kCurrentThread);
  EXPECT_EQ(0UL, poller_mock::POLLED);
}

// the local thread is considered managed if kernel::startup was run in it.
TEST(simple, dispatch_runs_inline_when_in_same_thread) {
  auto k = make< mock_kernel >();

  int i = 0;
  k->dispatch(kCurrentThread, [&i] { i = 42; });

  EXPECT_EQ(42, i);
}

TEST(simple, dispatch_runs_async_when_in_different_thread) {
  auto k = make< mock_kernel >();

  int i = 0;
  k->dispatch(kOtherThread, [&i] { i = 42; });

  EXPECT_EQ(0, i);

  k->run_core(kOtherThread);

  EXPECT_EQ(42, i);
}

TEST(simple, post_always_runs_async) {
  auto k = make< mock_kernel >();

  int i = 0;
  std::cout << 1 << std::endl;
  k->post(kCurrentThread, [&i] { i = 42; });
  std::cout << 2 << std::endl;

  EXPECT_EQ(0, i);

  k->run_core(kCurrentThread);

  EXPECT_EQ(42, i);
}

TEST(simple, dispatch_on_wrong_core_throws) {
  auto k = make< mock_kernel >();
  EXPECT_THROW(k->dispatch(3, [] {}), std::invalid_argument);
}

TEST(simple, post_on_wrong_core_throws) {
  auto k = make< mock_kernel >();
  EXPECT_THROW(k->post(3, [] {}), std::invalid_argument);
}

TEST(exception, in_dispatch_in_other_thread_shuts_down_kernel) {
  auto k = make< mock_kernel >();

  EXPECT_NO_THROW(
      k->dispatch(kOtherThread, [] { throw std::runtime_error("Foo"); }));
  EXPECT_NO_THROW(k->run_core(kOtherThread));
  EXPECT_TRUE(k->is_shut_down());
  EXPECT_THROW(k->await_shutdown(), std::runtime_error);
}

TEST(exception, in_dispatch_in_current_thread_shuts_down_kernel) {
  auto k = make< mock_kernel >();

  EXPECT_NO_THROW(
      k->dispatch(kCurrentThread, [] { throw std::runtime_error("Foo"); }));
  EXPECT_TRUE(k->is_shut_down());
  EXPECT_THROW(k->await_shutdown(), std::runtime_error);
}

TEST(exception, in_post_in_other_thread_shuts_down_kernel) {
  auto k = make< mock_kernel >();

  EXPECT_NO_THROW(
      k->post(kOtherThread, [] { throw std::runtime_error("Foo"); }));
  EXPECT_NO_THROW(k->run_core(kOtherThread));
  EXPECT_TRUE(k->is_shut_down());
  EXPECT_THROW(k->await_shutdown(), std::runtime_error);
}

TEST(exception, in_post_in_current_thread_shuts_down_kernel) {
  auto k = make< mock_kernel >();

  EXPECT_NO_THROW(
      k->post(kCurrentThread, [] { throw std::runtime_error("Foo"); }));
  EXPECT_NO_THROW(k->run_core(kCurrentThread));
  EXPECT_TRUE(k->is_shut_down());
  EXPECT_THROW(k->await_shutdown(), std::runtime_error);
}

TEST(exception, in_case_of_concurrency_the_last_exception_is_thrown) {
  {
    auto k = make< mock_kernel >();

    EXPECT_NO_THROW(
        k->post(kCurrentThread, [] { throw std::logic_error("Foo"); }));
    EXPECT_NO_THROW(
        k->post(kOtherThread, [] { throw std::runtime_error("Foo"); }));
    EXPECT_NO_THROW(k->run_core(kCurrentThread));
    EXPECT_NO_THROW(k->run_core(kOtherThread));
    EXPECT_TRUE(k->is_shut_down());
    EXPECT_THROW(k->await_shutdown(), std::runtime_error);
  }
  {
    auto k = make< mock_kernel >();

    EXPECT_NO_THROW(
        k->post(kCurrentThread, [] { throw std::logic_error("Foo"); }));
    EXPECT_NO_THROW(
        k->post(kOtherThread, [] { throw std::runtime_error("Foo"); }));
    EXPECT_NO_THROW(k->run_core(kOtherThread));
    EXPECT_NO_THROW(k->run_core(kCurrentThread));
    EXPECT_TRUE(k->is_shut_down());
    EXPECT_THROW(k->await_shutdown(), std::logic_error);
  }
}
