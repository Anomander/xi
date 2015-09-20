#pragma once

#include "xi/async/future/ForwardDefinitions.h"
#include "xi/async/future/detail/CallableApplier.h"
#include "xi/async/future/detail/FutureResult.h"

namespace xi {
namespace async {

  template < class T >
  class State {
    struct Unfulfilled {};

    variant< Unfulfilled, T, exception_ptr > _value = Unfulfilled{};

    struct ValueVisitor : public static_visitor< T > {
      T operator()(T& value) const { return move(value); }
      [[noreturn]] T operator()(exception_ptr& ex) const { rethrow_exception(move(ex)); }
      [[noreturn]] T operator()(Unfulfilled& ex) const { std::terminate(); }
    };

  protected:
    State() = default;
    auto& value() { return _value; }

  public:
    State(T&& value) : _value(move(value)) {}
    State(exception_ptr ex) : _value(move(ex)) {}
    State(State&&) = default;
    State& operator=(State&&) = default;
    State(State const&) = default;
    State& operator=(State const&) = default;

    bool isReady() const { return (nullptr == ext::get< Unfulfilled >(&_value)); }
    bool isException() const { return (nullptr != ext::get< exception_ptr >(&_value)); }

    T getValue();
    exception_ptr getException();

    template < class Func >
    auto setContinuation(Func&& f);

    template < class Func >
    auto setContinuation(mut< core::Executor > e, Func&& f);
  };

  template < class T >
  T State< T >::getValue() {
    assert(isReady());
    return apply_visitor(ValueVisitor{}, _value);
  }

  template < class T >
  exception_ptr State< T >::getException() {
    assert(isException());
    return ext::get< exception_ptr >(_value);
  }

  template < class T >
  template < class Func >
  auto State< T >::setContinuation(Func&& f) {
    using ResultType = FutureResult< Func, T >;
    if (isException()) {
      return makeReadyFuture< ResultType >(getException());
    }
    try {
      return makeReadyFuture< ResultType >(CallableApplier< T >::apply(forward< Func >(f), getValue()));
    } catch (...) {
      return makeReadyFuture< ResultType >(current_exception());
    }
  }

  template < class T >
  template < class Func >
  auto State< T >::setContinuation(mut< core::Executor > e, Func&& f) {
    using ResultType = FutureResult< Func, T >;
    if (isException()) {
      return makeReadyFuture< ResultType >(getException());
    }
    Promise< ResultType > promise;
    auto future = promise.getFuture();

    e->post([ promise = move(promise), f = forward< Func >(f), v = getValue() ]() mutable {
      try {
        promise.set(CallableApplier< T >::apply(forward< Func >(f), move(v)));
      } catch (...) {
        return promise.set(current_exception());
      }
    });

    return move(future);
  }
}
}
