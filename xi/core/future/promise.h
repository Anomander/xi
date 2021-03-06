#pragma once

#include "xi/core/future/detail/callable_applier.h"
#include "xi/core/future/exceptions.h"
#include "xi/core/future/future.h"

namespace xi {
namespace core {

  template < class T >
  class promise {
    own< shared_state< T > > _state = make< shared_state< T > >();
    enum {
      kInitial    = 1,
      kFutureMade = 1 << 1,
      kValueSet   = 1 << 2,
    };
    uint8_t _flags = 0;
    void set_flag(uint8_t flag) {
      _flags |= flag;
    }
    bool test_flag(uint8_t flag) const {
      return (_flags & flag) == flag;
    }

  public:
    promise()           = default;
    promise(promise &&) = default;
    promise &operator=(promise &&) = default;

    ~promise() noexcept {
      if (_state /* might've been moved out */ && !_state->is_ready()) {
        _state->set(make_exception_ptr(broken_promise_exception{}));
      }
    }

    future< T > get_future();

    void set(T value);
    void set(future< T > &&value);
    void set();
    void set(exception_ptr ex);

    template < class F,
               class... A,
               XI_REQUIRE_DECL(is_same< T, future_result< F, A... > >) >
    void apply(F &&, A &&...);
  };

  template < class T >
  future< T > promise< T >::get_future() {
    if (test_flag(kFutureMade)) {
      throw invalid_promise_exception();
    }
    set_flag(kFutureMade);
    return future< T >(share(_state));
  }

  template < class T >
  void promise< T >::set(T value) {
    if (test_flag(kValueSet)) {
      throw invalid_promise_exception();
    }
    set_flag(kValueSet);
    _state->set(move(value));
  }

  template < class T >
  void promise< T >::set(future< T > &&value) {
    if (test_flag(kValueSet)) {
      throw invalid_promise_exception();
    }
    set_flag(kValueSet);
    _state = move(value).extract_into_shared_state(move(_state));
  }

  template < class T >
  void promise< T >::set() {
    static_assert(is_same< meta::null, T >::value,
                  "set() can only be used on promise<>");
    set(meta::null{});
  }

  template < class T >
  void promise< T >::set(exception_ptr ex) {
    if (test_flag(kValueSet)) {
      throw invalid_promise_exception();
    }
    set_flag(kValueSet);
    _state->set(move(ex));
  }

  template < class T >
  template < class F,
             class... A,
             XI_REQUIRE(is_same< T, future_result< F, A... > >) >
  void promise< T >::apply(F &&func, A &&... args) {
    try {
      set(callable_applier< A... >::apply(func, forward< A >(args)...));
    } catch (...) {
      set(current_exception());
    }
  }
}
}
