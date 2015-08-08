#include <gtest/gtest.h>

#include "async/Future.h"
#include "async/Engine.h"
#include "async/Context.h"

using namespace xi;
using namespace xi::async;

struct UnitTestExecutor : public Executor {
  using Executor::Executor;
  void run(function< void() > f) override { setup(); }
};

ExecutorGroup< UnitTestExecutor > group(1, 1000);

class ExecutableTest : public ::testing::Test {
  using UnitTestExecutorGroup = ExecutorGroup< UnitTestExecutor >;

public:
  void SetUp() override {
    _executors = make< UnitTestExecutorGroup >(1, 1000);
    _executors->run([] {}); // initialize executor
  }

  mut< UnitTestExecutorGroup > executors() { return edit(_executors); }

  void pollEvents() { local< Executor >().processQueues(); }

private:
  own< UnitTestExecutorGroup > _executors;
};

template<class T>
T getValueFromFuture(Future<T> & f){
  T val;
  f.then(kInline, [&val](T v){ val = v; });
  return val;
}

TEST(Simple, FutureIsMoveConstructible) {
  auto fut1 = makeReadyFuture(1);
  ASSERT_TRUE(fut1.isReady());
  ASSERT_FALSE(fut1.isException());

  Future< int > fut2{move(fut1)};
  ASSERT_TRUE(fut2.isReady());
  ASSERT_FALSE(fut2.isException());
  ASSERT_FALSE(fut1.isReady());
  ASSERT_FALSE(fut1.isException());
}

TEST(Simple, FutureIsMoveAssignable) {
  auto fut1 = makeReadyFuture(1);
  ASSERT_TRUE(fut1.isReady());
  ASSERT_FALSE(fut1.isException());

  auto fut2 = move(fut1);
  ASSERT_TRUE(fut2.isReady());
  ASSERT_FALSE(fut2.isException());
  ASSERT_FALSE(fut1.isReady());
  ASSERT_FALSE(fut1.isException());
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
  p.setValue();
  ASSERT_FALSE(fut.isException());
  ASSERT_TRUE(fut.isReady());
}

TEST(Simple, PromiseSetValue) {
  Promise< int > p;
  auto fut = p.getFuture();
  ASSERT_FALSE(fut.isException());
  ASSERT_FALSE(fut.isReady());
  p.setValue(1);
  ASSERT_FALSE(fut.isException());
  ASSERT_TRUE(fut.isReady());
}

TEST(Simple, PromiseSetExceptionAsValue) {
  Promise< int > p;
  auto fut = p.getFuture();
  ASSERT_FALSE(fut.isException());
  ASSERT_FALSE(fut.isReady());
  p.setValue(make_exception_ptr(std::exception{}));
  ASSERT_TRUE(fut.isException());
  ASSERT_TRUE(fut.isReady());
}

TEST(Simple, PromiseSetException) {
  Promise< int > p;
  auto fut = p.getFuture();
  ASSERT_FALSE(fut.isException());
  ASSERT_FALSE(fut.isReady());
  p.setException(make_exception_ptr(std::exception{}));
  ASSERT_TRUE(fut.isException());
  ASSERT_TRUE(fut.isReady());
}

TEST(Simple, BrokenPromise) {
  auto f = Promise< int > {}.getFuture();
  ASSERT_TRUE(f.isReady());
  ASSERT_TRUE(f.isException());
}

TEST(Continuation, MultipleParameters) {
  auto f = makeReadyFuture(1, 3.1415926);
  int i = 0;
  double d = 0;
  auto r = f.then([&i, &d](int j, double pi) {
    i = j;
    d = pi;
  });
  ASSERT_EQ(1, i);
  ASSERT_EQ(3.1415926, d);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, MultipleParametersViaPromise) {
  Promise< int, double > p;
  auto f = p.getFuture();
  p.setValue(1, 3.1415926);
  int i = 0;
  double d = 0;
  auto r = f.then([&i, &d](int j, double pi) {
    i = j;
    d = pi;
  });
  ASSERT_EQ(1, i);
  ASSERT_EQ(3.1415926, d);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, IfReadyExecutesImmediately) {
  auto f = makeReadyFuture(1);
  int i = 0;
  auto r = f.then([&i](int j) { i = j; });
  ASSERT_EQ(1, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, IfReadyViaPromiseExecutesImmediately) {
  Promise< int > p;
  auto f = p.getFuture();
  p.setValue(1);
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

TEST(Continuation, SetExceptionViaFutureTriggersContinuation) {
  Promise< int > p;
  auto f = p.getFuture();
  int i = 0;
  auto r = f.then([&i](int j) {i = j * j; });

  ASSERT_NO_THROW(p.setException(make_exception_ptr(std::exception{})));

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST(Continuation, SetValueViaFutureTriggersContinuation) {
  Promise< int > p;
  auto f = p.getFuture();
  int i = 0;
  auto r = f.then([&i](int j) {i = j * j; });

  p.setValue(42);

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

  ASSERT_NO_THROW(p.setException(make_exception_ptr(std::exception{})));

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

  p.setValue(42);

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST(Continuation, ExceptionFromContinuationPropagates_Promise) {
  Promise< int > p;
  auto f = p.getFuture();

  auto r = f.then([](int j) { throw std::exception(); });

  p.setValue(42);

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

  p.setValue(42);

  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
  auto val = getValueFromFuture(r);
  ASSERT_EQ(42*42, val);
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

  p.setValue(42);

  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST_F(ExecutableTest, AsyncContinuationLaunchesAsynchronously_ReadyFuture) {
  int i = 0;
  auto r = makeReadyFuture(42).then(kAsync, [&i](int j) { i = j * j; });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  pollEvents();

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST_F(ExecutableTest, AsyncContinuationLaunchesAsynchronously_ReadyPromise) {
  int i = 0;
  Promise< int > p;
  auto f = p.getFuture();
  p.setValue(42);

  auto r = f.then(kAsync, [&i](int j) { i = j * j; });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  pollEvents();

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST_F(ExecutableTest, AsyncContinuationLaunchesAsynchronously_PromiseSetValue) {
  int i = 0;
  Promise< int > p;
  auto r = p.getFuture().then(kAsync, [&i](int j) { i = j * j; });
  p.setValue(42);

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  pollEvents();

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST_F(ExecutableTest, AsyncContinuationPropagatesExceptions_ReadyPromise) {
  int i = 0;
  auto r = makeReadyFuture(42).then(kAsync, [&i](int j) { throw std::exception(); });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  pollEvents();

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST_F(ExecutableTest, AsyncContinuationPropagatesExceptions_PromiseSetValue) {
  int i = 0;
  Promise< int > p;
  auto r = p.getFuture().then(kAsync, [&i](int j) { throw std::exception(); });
  p.setValue(42);

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  pollEvents();

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}

TEST_F(ExecutableTest, AsyncPromiseSetValue_PropagatesValue) {
  int i = 0;
  Promise< int > p;
  auto r = p.getFuture().then(kAsync, [&i](int j) { i = j * j; });
  schedule([p=move(p)]() mutable{ p.setValue(42); });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  pollEvents();
  pollEvents();

  ASSERT_EQ(42 * 42, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_FALSE(r.isException());
}

TEST_F(ExecutableTest, AsyncPromiseSetValue_PropagatesException) {
  int i = 0;
  Promise< int > p;
  auto r = p.getFuture().then(kAsync, [&i](int j) { throw std::exception(); });
  schedule([p=move(p)]() mutable{ p.setValue(42); });

  ASSERT_EQ(0, i);
  ASSERT_FALSE(r.isReady());
  ASSERT_FALSE(r.isException());

  pollEvents();

  ASSERT_EQ(0, i);
  ASSERT_TRUE(r.isReady());
  ASSERT_TRUE(r.isException());
}
