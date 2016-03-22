#pragma once

#include "xi/async/future/state.h"
#include "xi/util/spin_lock.h"

namespace xi {
namespace async {

  template < class T >
  class shared_state : public state< T >, public virtual ownership::std_shared {
    template < class func >
    struct owning_callback;

    template < class func >
    struct ref_callback;

    enum { kInlineCallbackStorageSize = 64 };
    char _callback_storage[kInlineCallbackStorageSize];

    function< void(shared_state *) > _callback;
    spin_lock _spin_lock;

    void maybe_callback();

    template < class func >
    void set_callback(func &&callback);

  public:
    void set(T value);

    void set(exception_ptr ex);

    template < class func >
    auto set_continuation(func &&f);

    template < class func >
    auto set_continuation(mut< core::shard > e, func &&f);
  };

  template < class T >
  void shared_state< T >::set(T value) {
    auto lock     = make_lock(_spin_lock);
    this->value() = move(value);
    maybe_callback();
  }

  template < class T >
  void shared_state< T >::set(exception_ptr ex) {
    auto lock     = make_lock(_spin_lock);
    this->value() = move(ex);
    maybe_callback();
  }

  template < class T >
  template < class func >
  auto shared_state< T >::set_continuation(func &&f) {
    auto lock = make_lock(_spin_lock);
    if (this->is_ready()) {
      return state< T >::set_continuation(forward< func >(f));
    }
    promise< future_result< func, T > > promise;
    auto future = promise.get_future();

    set_callback([ promise = move(promise),
                   f = forward< func >(f) ](mut< shared_state > state) mutable {
      if (state->is_exception()) {
        return promise.set(state->get_exception());
      }
      try {
        promise.set(callable_applier< T >::apply(forward< func >(f),
                                                 state->get_value()));
      } catch (...) {
        return promise.set(current_exception());
      }
    });

    return move(future);
  }

  template < class T >
  template < class func >
  auto shared_state< T >::set_continuation(mut< core::shard > e, func &&f) {
    auto lock = make_lock(_spin_lock);
    if (this->is_ready()) {
      return state< T >::set_continuation(e, forward< func >(f));
    }
    promise< future_result< func, T > > promise;
    auto future = promise.get_future();

    set_callback([ promise = move(promise), f = forward< func >(f), e ](
        mut< shared_state > state) mutable {
      if (state->is_exception()) {
        return promise.set(state->get_exception());
      }
      e->post([
        promise = move(promise),
        f       = forward< func >(f),
        v       = state->get_value()
      ]() mutable {
        try {
          promise.set(
              callable_applier< T >::apply(forward< func >(f), move(v)));
        } catch (...) {
          return promise.set(current_exception());
        }
      });
    });

    return move(future);
  }

  template < class T >
  template < class func >
  struct shared_state< T >::owning_callback {
    owning_callback(func &&f) : _f(make_unique< func >(move(f))){};

    void operator()(mut< shared_state > state) {
      _run(state);
    }

  private:
    void _run(mut< shared_state > state) {
      XI_SCOPE(exit) {
        _f.reset();
      };
      (*_f)(state);
    }
    unique_ptr< func > _f;
  };

  template < class T >
  template < class func >
  struct shared_state< T >::ref_callback {
    ref_callback(func &&f) : _f(move(f)){};

    void operator()(mut< shared_state > state) {
      _run(state);
    }

  private:
    void _run(mut< shared_state > state) {
      XI_SCOPE(exit) {
        _f.~func();
      };
      _f(state);
    }
    func _f;
  };
  template < class T >
  void shared_state< T >::maybe_callback() {
    if (_callback) {
      _callback(this);
    }
  }

  template < class T >
  template < class func >
  void shared_state< T >::set_callback(func &&callback) {
    if (kInlineCallbackStorageSize >= sizeof(ref_callback< func >)) {
      auto cb = new (_callback_storage)
          ref_callback< func >{forward< func >(callback)};
      _callback = reference(*cb);
    } else {
      auto cb = new (_callback_storage)
          owning_callback< func >{forward< func >(callback)};
      _callback = reference(*cb);
    }
  }
}
}
