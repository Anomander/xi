#include <gtest/gtest.h>

#include "xi/async/future.h"
#include "src/test/test_kernel.h"
#include "xi/core/kernel_utils.h"

using namespace xi;
using namespace xi::async;

template < class T > T get_value_from_future(future< T > &f) {
  T val;
  f.then([&val](T v) { val = v; });
  return val;
}

TEST(simple, future_is_move_constructible) {
  auto fut1 = make_ready_future(1);
  ASSERT_TRUE(fut1.is_ready());
  ASSERT_FALSE(fut1.is_exception());

  future< int > fut2{move(fut1)};
  ASSERT_TRUE(fut2.is_ready());
  ASSERT_FALSE(fut2.is_exception());
}

TEST(simple, future_is_move_assignable) {
  auto fut1 = make_ready_future(1);
  ASSERT_TRUE(fut1.is_ready());
  ASSERT_FALSE(fut1.is_exception());

  auto fut2 = move(fut1);
  ASSERT_TRUE(fut2.is_ready());
  ASSERT_FALSE(fut2.is_exception());
}

TEST(simple, make_ready_future_is_ready) {
  ASSERT_TRUE(make_ready_future(1).is_ready());
  ASSERT_TRUE(
      make_ready_future(make_exception_ptr(std::exception{})).is_ready());
}

TEST(simple, future_with_exception) {
  ASSERT_FALSE(make_ready_future(1).is_exception());
  ASSERT_TRUE(
      make_ready_future(make_exception_ptr(std::exception{})).is_exception());
}

TEST(simple, promise_set_void_value) {
  promise<> p;
  auto fut = p.get_future();
  ASSERT_FALSE(fut.is_exception());
  ASSERT_FALSE(fut.is_ready());
  p.set();
  ASSERT_FALSE(fut.is_exception());
  ASSERT_TRUE(fut.is_ready());
}

TEST(simple, promise_set_value) {
  promise< int > p;
  auto fut = p.get_future();
  ASSERT_FALSE(fut.is_exception());
  ASSERT_FALSE(fut.is_ready());
  p.set(1);
  ASSERT_FALSE(fut.is_exception());
  ASSERT_TRUE(fut.is_ready());
}

TEST(simple, promise_apply_non_void_function) {
  promise< int > p;
  auto fut = p.get_future();
  ASSERT_FALSE(fut.is_exception());
  ASSERT_FALSE(fut.is_ready());
  p.apply([] () -> int { return 42; });
  ASSERT_FALSE(fut.is_exception());
  ASSERT_TRUE(fut.is_ready());
}

TEST(simple, promise_apply_function_with_exception) {
  promise< int > p;
  auto fut = p.get_future();
  ASSERT_FALSE(fut.is_exception());
  ASSERT_FALSE(fut.is_ready());
  p.apply([] () -> int { throw std::exception(); });
  ASSERT_TRUE(fut.is_exception());
  ASSERT_TRUE(fut.is_ready());
}

TEST(simple, promise_apply_void_function) {
  promise< > p;
  auto fut = p.get_future();
  ASSERT_FALSE(fut.is_exception());
  ASSERT_FALSE(fut.is_ready());
  p.apply([] {});
  ASSERT_FALSE(fut.is_exception());
  ASSERT_TRUE(fut.is_ready());
}

TEST(simple, promise_set_exception_as_value) {
  promise< int > p;
  auto fut = p.get_future();
  ASSERT_FALSE(fut.is_exception());
  ASSERT_FALSE(fut.is_ready());
  p.set(make_exception_ptr(std::exception{}));
  ASSERT_TRUE(fut.is_exception());
  ASSERT_TRUE(fut.is_ready());
}

TEST(simple, promise_set_exception) {
  promise< int > p;
  auto fut = p.get_future();
  ASSERT_FALSE(fut.is_exception());
  ASSERT_FALSE(fut.is_ready());
  p.set(make_exception_ptr(std::exception{}));
  ASSERT_TRUE(fut.is_exception());
  ASSERT_TRUE(fut.is_ready());
}

TEST(simple, broken_promise) {
  auto f = promise< int >{}.get_future();
  ASSERT_TRUE(f.is_ready());
  ASSERT_TRUE(f.is_exception());
}

TEST(simple, promise_set_value_twice) {
  auto p = promise< int >{};
  auto f = p.get_future();
  ASSERT_NO_THROW(p.set(1));
  ASSERT_THROW(p.set(1), invalid_promise_exception);
  ASSERT_TRUE(f.is_ready());
  ASSERT_FALSE(f.is_exception());
}

TEST(simple, promise_get_future_twice) {
  auto p = promise< int >{};
  auto f1 = p.get_future();
  ASSERT_THROW(auto f2 = p.get_future(), invalid_promise_exception);
  ASSERT_NO_THROW(p.set(1));
  ASSERT_TRUE(f1.is_ready());
  ASSERT_FALSE(f1.is_exception());
}

TEST(continuation, if_ready_executes_immediately) {
  auto f = make_ready_future(1);
  int i = 0;
  auto r = f.then([&i](int j) { i = j; });
  ASSERT_EQ(1, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(continuation, if_ready_returns_correct_value) {
  auto f = make_ready_future(42);
  int i = 0;
  auto r = f.then([](int j) { return j * j; }).then([&i](int k) { i = k; });
  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(continuation, if_ready_with_exception_calls_correctly) {
  auto f = make_ready_future< int >(std::make_exception_ptr(std::exception{}));
  int i = 0;
  auto r = f.then([](int j) { return j * j; });
  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_TRUE(r.is_exception());
}

TEST(continuation, if_ready_via_promise_executes_immediately) {
  promise< int > p;
  auto f = p.get_future();
  p.set(1);
  int i = 0;
  auto r = f.then([&i](int j) { i = j; });
  ASSERT_EQ(1, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(continuation, set_exception_via_future_triggers_continuation) {
  promise< int > p;
  auto f = p.get_future();
  int i = 0;
  auto r = f.then([&i](int j) { i = j * j; });

  ASSERT_NO_THROW(p.set(make_exception_ptr(std::exception{})));

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_TRUE(r.is_exception());
}

TEST(continuation, set_value_via_future_triggers_continuation) {
  promise< int > p;
  auto f = p.get_future();
  int i = 0;
  auto r = f.then([&i](int j) { i = j * j; });

  p.set(42);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(continuation, set_exception_via_future_triggers_large_continuation) {
  promise< int > p;
  auto f = p.get_future();
  int i = 0;
  char padding[1024];
  auto continuation = [&i, padding](int j) { i = j * j; };
  static_assert(sizeof(continuation) > 1024, "Too small!");

  auto r = f.then(continuation);

  ASSERT_NO_THROW(p.set(make_exception_ptr(std::exception{})));

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_TRUE(r.is_exception());
}

TEST(continuation, set_value_via_future_triggers_large_continuation) {
  promise< int > p;
  auto f = p.get_future();
  int i = 0;
  char padding[1024];
  auto continuation = [&i, padding](int j) { i = j * j; };
  static_assert(sizeof(continuation) > 1024, "Too small!");

  auto r = f.then(continuation);

  p.set(42);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(continuation, exception_from_continuation_propagates_Promise) {
  promise< int > p;
  auto f = p.get_future();

  auto r = f.then([](int j) { throw std::exception(); });

  p.set(42);

  ASSERT_TRUE(r.is_ready());
  ASSERT_TRUE(r.is_exception());
}

TEST(continuation, exception_from_continuation_propagates_Future) {
  auto r = make_ready_future(42).then([](int j) { throw std::exception(); });

  ASSERT_TRUE(r.is_ready());
  ASSERT_TRUE(r.is_exception());
}

TEST(continuation, future_from_continuation_is_unwrapped_Future) {
  auto r = make_ready_future(42)
               .then([](int j) { return make_ready_future< int >(j * j); });

  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  auto val = get_value_from_future(r);
  ASSERT_EQ(42 * 42, val);
}

TEST(continuation, future_from_continuation_is_unwrapped_Promise) {
  promise< int > p;
  auto f = p.get_future();

  auto r = f.then([](int j) { return make_ready_future< int >(j * j); });

  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  p.set(42);

  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
  auto val = get_value_from_future(r);
  ASSERT_EQ(42 * 42, val);
}

TEST(continuation, future_from_continuation_is_unwrapped_Void_Future) {
  auto r =
      make_ready_future(42).then([](int j) { return make_ready_future(); });

  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(continuation, future_from_continuation_is_unwrapped_Void_Promise) {
  promise< int > p;
  auto f = p.get_future();

  auto r = f.then([](int j) { return make_ready_future(); });

  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  p.set(42);

  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(continuation, future_from_continuation_is_unwrapped_Exception_Future) {
  auto r = make_ready_future(42).then([](int j) { throw std::exception{}; });

  ASSERT_TRUE(r.is_ready());
  ASSERT_TRUE(r.is_exception());
}

TEST(continuation, future_from_continuation_is_unwrapped_Exception_Promise) {
  promise< int > p;
  auto f = p.get_future();

  auto r = f.then([](int j) { throw std::exception{}; });

  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  p.set(42);

  ASSERT_TRUE(r.is_ready());
  ASSERT_TRUE(r.is_exception());
}

TEST(executable_test, async_continuation_launches_asynchronously_Ready_future) {
  auto k = make< test::test_kernel >();
  auto pool = make_executor_pool(edit(k));

  int i = 0;
  auto r =
      make_ready_future(42).then(pool->executor_for_core(test::kCurrentThread),
                                 [&i](int j) { i = j * j; });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  k->run_core(test::kOtherThread);

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  k->run_core(test::kCurrentThread);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(executable_test,
     async_continuation_launches_asynchronously_Ready_promise) {
  auto k = make< test::test_kernel >();
  auto pool = make_executor_pool(edit(k));

  int i = 0;
  promise< int > p;
  auto f = p.get_future();
  p.set(42);

  auto r = f.then(pool->executor_for_core(test::kCurrentThread),
                  [&i](int j) { i = j * j; });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  k->run_core(test::kCurrentThread);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(executable_test,
     async_continuation_launches_asynchronously_Promise_set_value) {
  auto k = make< test::test_kernel >();
  auto pool = make_executor_pool(edit(k));

  int i = 0;
  promise< int > p;
  auto r = p.get_future().then(pool->executor_for_core(test::kCurrentThread),
                               [&i](int j) { i = j * j; });
  p.set(42);

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  k->run_core(test::kCurrentThread);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(executable_test, async_continuation_propagates_exceptions_Ready_promise) {
  auto k = make< test::test_kernel >();
  auto pool = make_executor_pool(edit(k));

  int i = 0;
  auto r =
      make_ready_future(42).then(pool->executor_for_core(test::kCurrentThread),
                                 [&i](int j) { throw std::exception(); });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  k->run_core(test::kCurrentThread);

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_TRUE(r.is_exception());
}

TEST(executable_test,
     async_continuation_propagates_exceptions_Promise_set_value) {
  auto k = make< test::test_kernel >();
  auto pool = make_executor_pool(edit(k));

  int i = 0;
  promise< int > p;
  auto r = p.get_future().then(pool->executor_for_core(test::kCurrentThread),
                               [&i](int j) { throw std::exception(); });
  p.set(42);

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  k->run_core(test::kCurrentThread);

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_TRUE(r.is_exception());
}

TEST(executable_test, async_promise_set_value_Propagates_value) {
  auto k = make< test::test_kernel >();
  auto pool = make_executor_pool(edit(k));

  int i = 0;
  promise< int > p;
  auto r = p.get_future().then(pool->executor_for_core(test::kCurrentThread),
                               [&i](int j) { i = j * j; });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  p.set(42);
  k->run_core(test::kCurrentThread);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_FALSE(r.is_exception());
}

TEST(executable_test, async_promise_set_value_Propagates_exception) {
  auto k = make< test::test_kernel >();
  auto pool = make_executor_pool(edit(k));

  int i = 0;
  promise< int > p;
  auto r = p.get_future().then(pool->executor_for_core(test::kCurrentThread),
                               [&i](int j) { throw std::exception(); });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.is_ready());
  ASSERT_FALSE(r.is_exception());

  p.set(make_exception_ptr(std::exception{}));
  k->run_core(test::kCurrentThread);

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.is_ready());
  ASSERT_TRUE(r.is_exception());
}
