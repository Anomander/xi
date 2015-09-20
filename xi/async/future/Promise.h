#pragma once

#include "xi/async/future/Exceptions.h"
#include "xi/async/future/Future.h"

namespace xi {
namespace async {

  template < class T >
  class Promise {
    own< SharedState< T > > _state = make< SharedState< T > >();
    enum {
      kInitial = 1,
      kFutureMade = 1 << 1,
      kValueSet = 1 << 2,
    };
    uint8_t _flags = 0;
    void setFlag(uint8_t flag) { _flags |= flag; }
    bool testFlag(uint8_t flag) const { return (_flags & flag) == flag; }

  public:
    Promise() = default;
    Promise(Promise&&) = default;
    Promise& operator=(Promise&&) = default;

    ~Promise() noexcept {
      if (_state /* might've been moved out */ && !_state->isReady()) {
        _state->set(make_exception_ptr(BrokenPromiseException{}));
      }
    }

    Future< T > getFuture();

    void set(T value);
    void set(Future< T >&& value);
    void set();
    void set(exception_ptr ex);
  };

  template<class T>
  Future< T > Promise<T>::getFuture() {
    if (testFlag(kFutureMade)) {
      throw InvalidPromiseException();
    }
    setFlag(kFutureMade);
    return Future< T >(share(_state));
  }

  template<class T>
  void Promise<T>::set(T value) {
    if (testFlag(kValueSet)) {
      throw InvalidPromiseException();
    }
    setFlag(kValueSet);
    _state->set(move(value));
  }

  template<class T>
  void Promise<T>::set(Future< T >&& value) {
    if (testFlag(kValueSet)) {
      throw InvalidPromiseException();
    }
    setFlag(kValueSet);
    _state = move(value).extractIntoSharedState(move(_state));
  }

  template<class T>
  void Promise<T>::set() {
    static_assert(is_same< meta::Null, T >::value, "set() can only be used on Promise<>");
    set(meta::Null{});
  }

  template<class T>
  void Promise<T>::set(exception_ptr ex) {
    if (testFlag(kValueSet)) {
      throw InvalidPromiseException();
    }
    setFlag(kValueSet);
    _state->set(move(ex));
  }
}
}
