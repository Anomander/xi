#pragma once

#include "xi/async/future/State.h"
#include "xi/util/SpinLock.h"

namespace xi {
namespace async {

  template < class T >
  class SharedState : public State< T >, public virtual ownership::StdShared {
    template < class Func >
    struct OwningCallback;

    template < class Func >
    struct RefCallback;

    enum { kInlineCallbackStorage = 64 };
    char _callbackStorage[kInlineCallbackStorage];

    function< void(SharedState*)> _callback;
    SpinLock _spinLock;

    void maybeCallback();

    template < class Func >
    void setCallback(Func&& callback);

  public:
    void set(T value);

    void set(exception_ptr ex);

    template < class Func >
    auto setContinuation(Func&& f);

    template < class Func >
    auto setContinuation(mut< core::Executor > e, Func&& f);
  };

  template < class T >
  void SharedState< T >::set(T value) {
    auto lock = makeLock(_spinLock);
    this->value() = move(value);
    maybeCallback();
  }

  template < class T >
  void SharedState< T >::set(exception_ptr ex) {
    auto lock = makeLock(_spinLock);
    this->value() = move(ex);
    maybeCallback();
  }

  template < class T >
  template < class Func >
  auto SharedState< T >::setContinuation(Func&& f) {
    auto lock = makeLock(_spinLock);
    if (this->isReady()) {
      return State< T >::setContinuation(forward< Func >(f));
    }
    Promise< FutureResult< Func, T > > promise;
    auto future = promise.getFuture();

    setCallback([ promise = move(promise), f = forward< Func >(f) ](mut< SharedState > state) mutable {
      if (state->isException()) {
        return promise.set(state->getException());
      }
      try {
        promise.set(CallableApplier< T >::apply(forward< Func >(f), state->getValue()));
      } catch (...) {
        return promise.set(current_exception());
      }
    });

    return move(future);
  }

  template < class T >
  template < class Func >
  auto SharedState< T >::setContinuation(mut< core::Executor > e, Func&& f) {
    auto lock = makeLock(_spinLock);
    if (this->isReady()) {
      return State< T >::setContinuation(e, forward< Func >(f));
    }
    Promise< FutureResult< Func, T > > promise;
    auto future = promise.getFuture();

    setCallback([ promise = move(promise), f = forward< Func >(f), e ](mut< SharedState > state) mutable {
      if (state->isException()) {
        return promise.set(state->getException());
      }
      e->post([ promise = move(promise), f = forward< Func >(f), v = state->getValue() ]() mutable {
        try {
          promise.set(CallableApplier< T >::apply(forward< Func >(f), move(v)));
        } catch (...) {
          return promise.set(current_exception());
        }
      });
    });

    return move(future);
  }

  template < class T >
  template < class Func >
  struct SharedState< T >::OwningCallback {
    OwningCallback(Func&& f) : _f(make_unique< Func >(move(f))){};

    void operator()(mut< SharedState > state) { _run(state); }

  private:
    void _run(mut< SharedState > state) {
      XI_SCOPE(exit) { _f.reset(); };
      (*_f)(state);
    }
    unique_ptr< Func > _f;
  };

  template < class T >
  template < class Func >
  struct SharedState< T >::RefCallback {
    RefCallback(Func&& f) : _f(move(f)){};

    void operator()(mut< SharedState > state) { _run(state); }

  private:
    void _run(mut< SharedState > state) {
      XI_SCOPE(exit) { _f.~Func(); };
      _f(state);
    }
    Func _f;
  };
  template < class T >
  void SharedState< T >::maybeCallback() {
    if (_callback) {
      _callback(this);
    }
  }

  template < class T >
  template < class Func >
  void SharedState< T >::setCallback(Func&& callback) {
    if (kInlineCallbackStorage >= sizeof(RefCallback< Func >)) {
      auto cb = new (_callbackStorage) RefCallback< Func >{forward< Func >(callback)};
      _callback = reference(*cb);
    } else {
      auto cb = new (_callbackStorage) OwningCallback< Func >{forward< Func >(callback)};
      _callback = reference(*cb);
    }
  }
}
}
