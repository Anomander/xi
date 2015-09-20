#pragma once

#include "xi/async/future/SharedState.h"

namespace xi {
namespace async {

  static constexpr struct ReadyMadeTag {
  } ReadyMade{};

  template < class T >
  class Future {
    variant< State< T >, own< SharedState< T > > > _state;

    State< T > const* state() const { return const_cast< Future* >(this)->state(); }
    State< T >* state() {
      auto* local = ext::get< State< T > >(&_state);
      if (local) {
        return local;
      }
      return addressOf(ext::get< own< SharedState< T > > >(_state));
    }

    Future(ReadyMadeTag, T&& value) : _state(move(value)) {}
    Future(ReadyMadeTag, exception_ptr&& ex) : _state(move(ex)) {}
    Future(own< SharedState< T > > value) : _state(move(value)) {}

    template < class U >
    friend Future< U > makeReadyFuture(U&& value);
    friend Future<> makeReadyFuture();
    template < class U >
    friend Future< U > makeReadyFuture(exception_ptr ex);
    friend Future<> makeReadyFuture(exception_ptr ex);

    friend class Promise< T >;

    struct IsReadyVisitor : public static_visitor< bool > {
      bool operator()(State< T > const& state) const { return state.isReady(); }
      bool operator()(own< SharedState< T > > const& state) const { return state->isReady(); }
    };
    struct IsExceptionVisitor : public static_visitor< bool > {
      bool operator()(State< T > const& state) const { return state.isException(); }
      bool operator()(own< SharedState< T > > const& state) const { return state->isException(); }
    };

    own< SharedState< T > > extractIntoSharedState(own< SharedState< T > > st) && {
      if (isReady()) {
        st->set(state()->getValue());
        return move(st);
      } else {
        return ext::get< own< SharedState< T > > >(_state);
      }
    }

  public:
    Future(Future&&) = default;
    Future& operator=(Future&&) = default;

    bool isReady() const { return apply_visitor(IsReadyVisitor{}, _state); }
    bool isException() const { return apply_visitor(IsExceptionVisitor{}, _state); }

    template < class Func >
    decltype(auto) then(Func&& f) {
      auto* local = ext::get< State< T > >(&_state);
      if (local) {
        return local->setContinuation(forward< Func >(f));
      }
      return ext::get< own< SharedState< T > > >(_state)->setContinuation(forward< Func >(f));
    }

    template < class Func >
    decltype(auto) then(mut< core::Executor > e, Func&& f) {
      auto* local = ext::get< State< T > >(&_state);
      if (local) {
        return local->setContinuation(e, forward< Func >(f));
      }
      return ext::get< own< SharedState< T > > >(_state)->setContinuation(e, forward< Func >(f));
    }
  };

  template <>
  class Future< void > : public Future<> {
  public:
    using Future<>::Future;
  };

  template < class T >
  Future< T > makeReadyFuture(T&& value) {
    return Future< T >(ReadyMade, forward< T >(value));
  }
  Future<> makeReadyFuture() { return Future<>(ReadyMade, meta::Null{}); }
  Future<> makeReadyFuture(meta::Null) { return makeReadyFuture(); }
  template < class T >
  Future< T > makeReadyFuture(exception_ptr ex) {
    return Future< T >(ReadyMade, forward< exception_ptr >(ex));
  }
  Future<> makeReadyFuture(exception_ptr ex) { return Future<>(ReadyMade, forward< exception_ptr >(ex)); }
  template < class U >
  Future< U > makeReadyFuture(Future< U >&& value) {
    return move(value);
  }
}
}
