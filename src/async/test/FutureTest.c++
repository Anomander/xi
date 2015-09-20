#include <gtest/gtest.h>

#include "xi/async/Future.h"
#include "src/test/TestKernel.h"
#include "xi/core/KernelUtils.h"

using namespace xi;
using namespace xi::async;

template<class T>
T getValueFromFuture(Future<T> & f){
  T val;
  f.then([&val](T v){ val = v; });
  return val;
}

TEST(Simple, FutureIsMoveConstructible) {
  auto fut1 = makeReadyFuture(1);
  ASSERT_TRUE(fut1.isReady());
  ASSERT_FALSE(fut1.isException());

  Future< int > fut2{move(fut1)};
  ASSERT_TRUE(fut2.isReady());
  ASSERT_FALSE(fut2.isException());
}

TEST(Simple, FutureIsMoveAssignable) {
  auto fut1 = makeReadyFuture(1);
  ASSERT_TRUE(fut1.isReady());
  ASSERT_FALSE(fut1.isException());

  auto fut2 = move(fut1);
  ASSERT_TRUE(fut2.isReady());
  ASSERT_FALSE(fut2.isException());
}

TEST(Simple, MakeReadyFutureIsReady) {
  ASSERT_TRUE(makeReadyFuture(1).isReady());
  ASSERT_TRUE(makeReadyFuture(make_exception_ptr(std::exception{})).isReady());
}

TEST(Simple, FutureWithException) {
  ASSERT_FALSE(makeReadyFuture(1).isException());
  ASSERT_TRUE(makeReadyFuture(make_exception_ptr(std::exception{})).isException());
}

TEST(Simple, PromiseSetVoidValue) {
  Promise< > p;
  auto fut = p.getFuture();
  ASSERT_FALSE(fut.isException());
  ASSERT_FALSE(fut.isReady());
  p.set();
  ASSERT_FALSE(fut.isException());
  ASSERT_TRUE(fut.isReady());
}

TEST(Simple, PromiseSetValue) {
  Promise< int > p;
  auto fut = p.getFuture();
  ASSERT_FALSE(fut.isException());
  ASSERT_FALSE(fut.isReady());
  p.set(1);
  ASSERT_FALSE(fut.isException());
  ASSERT_TRUE(fut.isReady());
}

TEST(Simple, PromiseSetExceptionAsValue) {
  Promise< int > p;
  auto fut = p.getFuture();
  ASSERT_FALSE(fut.isException());
  ASSERT_FALSE(fut.isReady());
  p.set(make_exception_ptr(std::exception{}));
  ASSERT_TRUE(fut.isException());
  ASSERT_TRUE(fut.isReady());
}

TEST(Simple, PromiseSetException) {
  Promise< int > p;
  auto fut = p.getFuture();
  ASSERT_FALSE(fut.isException());
  ASSERT_FALSE(fut.isReady());
  p.set(make_exception_ptr(std::exception{}));
  ASSERT_TRUE(fut.isException());
  ASSERT_TRUE(fut.isReady());
}

TEST(Simple, BrokenPromise) {
  auto f = Promise< int > {}.getFuture();
  ASSERT_TRUE(f.isReady());
  ASSERT_TRUE(f.isException());
}

TEST(Simple, PromiseSetValueTwice) {
  auto p = Promise< int > {};
  auto f = p.getFuture();
  ASSERT_NO_THROW(p.set(1));
  ASSERT_THROW(p.set(1), InvalidPromiseException);
  ASSERT_TRUE(f.isReady());
  ASSERT_FALSE(f.isException());
}

TEST(Simple, PromiseGetFutureTwice) {
  auto p = Promise< int > {};
  auto f1 = p.getFuture();
  ASSERT_THROW(auto f2 = p.getFuture(), InvalidPromiseException);
  ASSERT_NO_THROW(p.set(1));
  ASSERT_TRUE(f1.isReady());
  ASSERT_FALSE(f1.isException());
}

TEST(Continuation, IfReadyExecutesImmediately) {
  auto f = makeReadyFuture(1);
  int i = 0;
  auto r = f.then([&i](int j) { i = j; });
  ASSERT_EQ(1, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, IfReadyReturnsCorrectValue) {
  auto f = makeReadyFuture(42);
  int i = 0;
  auto r = f.then([](int j) { return j * j; }).then([&i](int k) { i = k; });
  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, IfReadyWithExceptionCallsCorrectly) {
  auto f = makeReadyFuture<int>(std::make_exception_ptr(std::exception{}));
  int i = 0;
  auto r = f.then([](int j) { return j * j; });
  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST(Continuation, IfReadyViaPromiseExecutesImmediately) {
  Promise< int > p;
  auto f = p.getFuture();
  p.set(1);
  int i = 0;
  auto r = f.then([&i](int j) { i = j; });
  ASSERT_EQ(1, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, SetExceptionViaFutureTriggersContinuation) {
  Promise< int > p;
  auto f = p.getFuture();
  int i = 0;
  auto r = f.then([&i](int j) {i = j * j; });

  ASSERT_NO_THROW(p.set(make_exception_ptr(std::exception{})));

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST(Continuation, SetValueViaFutureTriggersContinuation) {
  Promise< int > p;
  auto f = p.getFuture();
  int i = 0;
  auto r = f.then([&i](int j) {i = j * j; });

  p.set(42);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, SetExceptionViaFutureTriggersLargeContinuation) {
  Promise< int > p;
  auto f = p.getFuture();
  int i = 0;
  char padding [1024];
  auto continuation = [&i, padding](int j) {i = j * j; };
  static_assert (sizeof(continuation) > 1024, "Too small!");

  auto r = f.then(continuation);

  ASSERT_NO_THROW(p.set(make_exception_ptr(std::exception{})));

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST(Continuation, SetValueViaFutureTriggersLargeContinuation) {
  Promise< int > p;
  auto f = p.getFuture();
  int i = 0;
  char padding [1024];
  auto continuation = [&i, padding](int j) {i = j * j; };
  static_assert (sizeof(continuation) > 1024, "Too small!");

  auto r = f.then(continuation);

  p.set(42);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, ExceptionFromContinuationPropagates_Promise) {
  Promise< int > p;
  auto f = p.getFuture();

  auto r = f.then([](int j) { throw std::exception(); });

  p.set(42);

  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST(Continuation, ExceptionFromContinuationPropagates_Future) {
  auto r = makeReadyFuture(42).then([](int j) { throw std::exception(); });

  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST(Continuation, FutureFromContinuationIsUnwrapped_Future) {
  auto r = makeReadyFuture(42).then([](int j) { return makeReadyFuture<int>(j * j); });

  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());

  auto val = getValueFromFuture(r);
  ASSERT_EQ(42*42, val);
}

TEST(Continuation, FutureFromContinuationIsUnwrapped_Promise) {
  Promise< int > p;
  auto f = p.getFuture();

  auto r = f.then([](int j) { return makeReadyFuture<int>(j * j); });

  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  p.set(42);

  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
  auto val = getValueFromFuture(r);
  ASSERT_EQ(42*42, val);
}

TEST(Continuation, FutureFromContinuationIsUnwrapped_Void_Future) {
  auto r = makeReadyFuture(42).then([](int j) { return makeReadyFuture(); });

  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, FutureFromContinuationIsUnwrapped_Void_Promise) {
  Promise< int > p;
  auto f = p.getFuture();

  auto r = f.then([](int j) { return makeReadyFuture(); });

  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  p.set(42);

  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, FutureFromContinuationIsUnwrapped_Exception_Future) {
  auto r = makeReadyFuture(42).then([](int j) { throw std::exception{}; });

  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST(Continuation, FutureFromContinuationIsUnwrapped_Exception_Promise) {
  Promise< int > p;
  auto f = p.getFuture();

  auto r = f.then([](int j) { throw std::exception{}; });

  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  p.set(42);

  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST(ExecutableTest, AsyncContinuationLaunchesAsynchronously_ReadyFuture) {
  auto k = make <test::TestKernel>();
  auto pool = makeExecutorPool(edit(k));

  int i = 0;
  auto r = makeReadyFuture(42).then(pool->executor(test::kCurrentThread), [&i](int j) { i = j * j; });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  k->runCore(test::kOtherThread);

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  k->runCore(test::kCurrentThread);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(ExecutableTest, AsyncContinuationLaunchesAsynchronously_ReadyPromise) {
  auto k = make <test::TestKernel>();
  auto pool = makeExecutorPool(edit(k));

  int i = 0;
  Promise< int > p;
  auto f = p.getFuture();
  p.set(42);

  auto r = f.then(pool->executor(test::kCurrentThread), [&i](int j) { i = j * j; });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  k->runCore(test::kCurrentThread);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(ExecutableTest, AsyncContinuationLaunchesAsynchronously_PromiseSetValue) {
  auto k = make <test::TestKernel>();
  auto pool = makeExecutorPool(edit(k));

  int i = 0;
  Promise< int > p;
  auto r = p.getFuture().then(pool->executor(test::kCurrentThread), [&i](int j) { i = j * j; });
  p.set(42);

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  k->runCore(test::kCurrentThread);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(ExecutableTest, AsyncContinuationPropagatesExceptions_ReadyPromise) {
  auto k = make <test::TestKernel>();
  auto pool = makeExecutorPool(edit(k));

  int i = 0;
  auto r = makeReadyFuture(42).then(pool->executor(test::kCurrentThread), [&i](int j) { throw std::exception(); });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  k->runCore(test::kCurrentThread);

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST(ExecutableTest, AsyncContinuationPropagatesExceptions_PromiseSetValue) {
  auto k = make <test::TestKernel>();
  auto pool = makeExecutorPool(edit(k));

  int i = 0;
  Promise< int > p;
  auto r = p.getFuture().then(pool->executor(test::kCurrentThread), [&i](int j) { throw std::exception(); });
  p.set(42);

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  k->runCore(test::kCurrentThread);

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST(ExecutableTest, AsyncPromiseSetValue_PropagatesValue) {
  auto k = make <test::TestKernel>();
  auto pool = makeExecutorPool(edit(k));

  int i = 0;
  Promise< int > p;
  auto r = p.getFuture().then(pool->executor(test::kCurrentThread), [&i](int j) { i = j * j; });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  p.set(42);
  k->runCore(test::kCurrentThread);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(ExecutableTest, AsyncPromiseSetValue_PropagatesException) {
  auto k = make <test::TestKernel>();
  auto pool = makeExecutorPool(edit(k));

  int i = 0;
  Promise< int > p;
  auto r = p.getFuture().then(pool->executor(test::kCurrentThread), [&i](int j) { throw std::exception(); });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  p.set(make_exception_ptr(std::exception{}));
  k->runCore(test::kCurrentThread);

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}
