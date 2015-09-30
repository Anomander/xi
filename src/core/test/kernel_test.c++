#include <gtest/gtest.h>

#include "xi/core/kernel.h"
#include "xi/hw/hardware.h"
#include "src/test/test_kernel.h"
#include "util.h"

using namespace xi;
using namespace xi::core;
using namespace xi::test;

TEST(core_id, local_core_id) {
  auto k = make< test_kernel >();
  ASSERT_EQ(kCurrentThread, k->local_core_id().unwrap());

  unsigned id = 1 << 15;
  k->post(kOtherThread, [&]() mutable { id = k->local_core_id().unwrap(); });
  k->run_core(kOtherThread);
  ASSERT_EQ(kOtherThread, id);
}

TEST(core_id, core_count_correct) {
  auto machine = xi::hw::enumerate();
  unsigned i = 1;
  for (; i <= machine.cpus().size(); ++i) {
    auto k = make< kernel >();
    k->start(i, 1000);
    ASSERT_EQ(i, k->core_count());
  }
  auto k = make< kernel >();
  ASSERT_THROW(k->start(i, 1000), std::invalid_argument);
  ASSERT_THROW(k->start(0U, 1000), std::invalid_argument);
}

struct poller_mock : public test::object_tracker, public poller {
  unsigned poll() noexcept override {
    ++POLLED;
    return 0;
  }

  poller_mock() = default;
  poller_mock(poller_mock const &) = delete;
  poller_mock(poller_mock &&) = delete;

  static void reset() {
    POLLED = 0;
    test::object_tracker::reset();
  }

  static size_t POLLED;
};

size_t poller_mock::POLLED = 0UL;

struct poller_test : public ::testing::Test {
  void SetUp() override { poller_mock::reset(); }
  void TearDown() override { poller_mock::reset(); }
};

TEST_F(poller_test, poller_only_runs_in_own_thread) {
  auto k = make< test_kernel >();
  k->register_poller(kCurrentThread, make< poller_mock >());
  ASSERT_EQ(0UL, poller_mock::POLLED);
  k->run_core(kOtherThread);
  ASSERT_EQ(0UL, poller_mock::POLLED);
  k->run_core(kCurrentThread);
  ASSERT_EQ(1UL, poller_mock::POLLED);
}

TEST_F(poller_test, register_poller) {
  auto k = make< test_kernel >();
  k->register_poller(kCurrentThread, make< poller_mock >());
  ASSERT_EQ(0UL, poller_mock::POLLED);
  k->run_core(kCurrentThread);
  ASSERT_EQ(1UL, poller_mock::POLLED);
}

TEST_F(poller_test, deregister_poller) {
  ASSERT_EQ(0UL, poller_mock::DESTROYED);
  auto k = make< test_kernel >();
  auto id = k->register_poller(kCurrentThread, make< poller_mock >());
  ASSERT_EQ(0UL, poller_mock::POLLED);
  k->run_core(kCurrentThread);
  ASSERT_EQ(1UL, poller_mock::POLLED);
  k->deregister_poller(kCurrentThread, id);
  ASSERT_EQ(1UL, poller_mock::DESTROYED);
  poller_mock::reset();
  k->run_core(kCurrentThread);
  ASSERT_EQ(0UL, poller_mock::POLLED);
}

// the local thread is considered managed if kernel::startup was run in it.
TEST(simple, dispatch_runs_inline_when_in_same_thread) {
  auto k = make< test_kernel >();

  int i = 0;
  k->dispatch(kCurrentThread, [&i] { i = 42; });

  ASSERT_EQ(42, i);
}

TEST(simple, dispatch_runs_async_when_in_different_thread) {
  auto k = make< test_kernel >();

  int i = 0;
  k->dispatch(kOtherThread, [&i] { i = 42; });

  ASSERT_EQ(0, i);

  k->run_core(kOtherThread);

  ASSERT_EQ(42, i);
}

TEST(simple, post_always_runs_async) {
  auto k = make< test_kernel >();

  int i = 0;
  k->post(kCurrentThread, [&i] { i = 42; });

  ASSERT_EQ(0, i);

  k->run_core(kCurrentThread);

  ASSERT_EQ(42, i);
}

TEST(simple, dispatch_on_wrong_core_throws) {
  auto k = make< test_kernel >();
  ASSERT_THROW(k->dispatch(3, [] {}), std::invalid_argument);
}

TEST(simple, post_on_wrong_core_throws) {
  auto k = make< test_kernel >();
  ASSERT_THROW(k->post(3, [] {}), std::invalid_argument);
}

TEST(exception, in_dispatch_in_other_thread_shuts_down_kernel) {
  auto k = make< test_kernel >();

  ASSERT_NO_THROW(
      k->dispatch(kOtherThread, [] { throw std::runtime_error("Foo"); }));
  ASSERT_NO_THROW(k->run_core(kOtherThread));
  ASSERT_TRUE(k->is_shut_down());
  ASSERT_THROW(k->await_shutdown(), std::runtime_error);
}

TEST(exception, in_dispatch_in_current_thread_shuts_down_kernel) {
  auto k = make< test_kernel >();

  ASSERT_NO_THROW(
      k->dispatch(kCurrentThread, [] { throw std::runtime_error("Foo"); }));
  ASSERT_TRUE(k->is_shut_down());
  ASSERT_THROW(k->await_shutdown(), std::runtime_error);
}

TEST(exception, in_post_in_other_thread_shuts_down_kernel) {
  auto k = make< test_kernel >();

  ASSERT_NO_THROW(
      k->post(kOtherThread, [] { throw std::runtime_error("Foo"); }));
  ASSERT_NO_THROW(k->run_core(kOtherThread));
  ASSERT_TRUE(k->is_shut_down());
  ASSERT_THROW(k->await_shutdown(), std::runtime_error);
}

TEST(exception, in_post_in_current_thread_shuts_down_kernel) {
  auto k = make< test_kernel >();

  ASSERT_NO_THROW(
      k->post(kCurrentThread, [] { throw std::runtime_error("Foo"); }));
  ASSERT_NO_THROW(k->run_core(kCurrentThread));
  ASSERT_TRUE(k->is_shut_down());
  ASSERT_THROW(k->await_shutdown(), std::runtime_error);
}

TEST(exception, in_case_of_concurrency_the_last_exception_is_thrown) {
  {
    auto k = make< test_kernel >();

    ASSERT_NO_THROW(
        k->post(kCurrentThread, [] { throw std::logic_error("Foo"); }));
    ASSERT_NO_THROW(
        k->post(kOtherThread, [] { throw std::runtime_error("Foo"); }));
    ASSERT_NO_THROW(k->run_core(kCurrentThread));
    ASSERT_NO_THROW(k->run_core(kOtherThread));
    ASSERT_TRUE(k->is_shut_down());
    ASSERT_THROW(k->await_shutdown(), std::runtime_error);
  }
  {
    auto k = make< test_kernel >();

    ASSERT_NO_THROW(
        k->post(kCurrentThread, [] { throw std::logic_error("Foo"); }));
    ASSERT_NO_THROW(
        k->post(kOtherThread, [] { throw std::runtime_error("Foo"); }));
    ASSERT_NO_THROW(k->run_core(kOtherThread));
    ASSERT_NO_THROW(k->run_core(kCurrentThread));
    ASSERT_TRUE(k->is_shut_down());
    ASSERT_THROW(k->await_shutdown(), std::logic_error);
  }
}
