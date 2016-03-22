#pragma once

#include "xi/async/future/detail/callable_applier.h"
#include "xi/async/future/detail/future_result.h"
#include "xi/async/future/forward_definitions.h"

namespace xi {
namespace async {

  template < class T >
  class state {
    struct unfulfilled {};

    variant< unfulfilled, T, exception_ptr > _value = unfulfilled{};

    struct value_visitor : public static_visitor< T > {
      T operator()(T &value) const {
        return move(value);
      }
      [[noreturn]] T operator()(exception_ptr &ex) const {
        rethrow_exception(move(ex));
      }
      [[noreturn]] T operator()(unfulfilled &ex) const {
        std::terminate();
      }
    };

  protected:
    state() = default;
    auto &value() {
      return _value;
    }

  public:
    state(T &&value) : _value(move(value)) {
    }
    state(exception_ptr ex) : _value(move(ex)) {
    }
    state(state &&) = default;
    state &operator=(state &&) = default;
    state(state const &)       = default;
    state &operator=(state const &) = default;

    bool is_ready() const {
      return (nullptr == ext::get< unfulfilled >(&_value));
    }
    bool is_exception() const {
      return (nullptr != ext::get< exception_ptr >(&_value));
    }

    T get_value();
    exception_ptr get_exception();

    template < class func >
    auto set_continuation(func &&f);

    template < class func >
    auto set_continuation(mut< core::shard > e, func &&f);
  };

  template < class T >
  T state< T >::get_value() {
    assert(is_ready());
    return apply_visitor(value_visitor{}, _value);
  }

  template < class T >
  exception_ptr state< T >::get_exception() {
    assert(is_exception());
    return ext::get< exception_ptr >(_value);
  }

  template < class T >
  template < class func >
  auto state< T >::set_continuation(func &&f) {
    using result_type = future_result< func, T >;
    if (is_exception()) {
      return make_ready_future< result_type >(get_exception());
    }
    try {
      return make_ready_future< result_type >(
          callable_applier< T >::apply(forward< func >(f), get_value()));
    } catch (...) {
      return make_ready_future< result_type >(current_exception());
    }
  }

  template < class T >
  template < class func >
  auto state< T >::set_continuation(mut< core::shard > e, func &&f) {
    using result_type = future_result< func, T >;
    if (is_exception()) {
      return make_ready_future< result_type >(get_exception());
    }
    promise< result_type > promise;
    auto future = promise.get_future();

    e->post([
      promise = move(promise),
      f       = forward< func >(f),
      v       = get_value()
    ]() mutable {
      try {
        promise.set(callable_applier< T >::apply(forward< func >(f), move(v)));
      } catch (...) {
        return promise.set(current_exception());
      }
    });

    return move(future);
  }
}
}
