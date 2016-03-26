#pragma once

#include "xi/core/future/shared_state.h"

namespace xi {
namespace core {

  static constexpr struct ready_made_tag {
  } ready_made{};

  template < class T >
  class future {
    variant< state< T >, own< shared_state< T > > > _state;

    state< T > const *get_state() const {
      return const_cast< future * >(this)->state();
    }
    state< T > *get_state() {
      auto *local = ext::get< state< T > >(&_state);
      if (local) {
        return local;
      }
      return address_of(ext::get< own< shared_state< T > > >(_state));
    }

    future(ready_made_tag, T &&value) : _state(move(value)) {
    }
    future(ready_made_tag, exception_ptr &&ex) : _state(move(ex)) {
    }
    future(own< shared_state< T > > value) : _state(move(value)) {
    }

    template < class U >
    friend future< U > make_ready_future(U &&value);
    friend future<> make_ready_future();
    template < class U >
    friend future< U > make_ready_future(exception_ptr ex);
    friend future<> make_ready_future(exception_ptr ex);

    friend class promise< T >;

    struct is_ready_visitor : public static_visitor< bool > {
      bool operator()(state< T > const &state) const {
        return state.is_ready();
      }
      bool operator()(own< shared_state< T > > const &state) const {
        return state->is_ready();
      }
    };
    struct is_exception_visitor : public static_visitor< bool > {
      bool operator()(state< T > const &state) const {
        return state.is_exception();
      }
      bool operator()(own< shared_state< T > > const &state) const {
        return state->is_exception();
      }
    };

    own< shared_state< T > > extract_into_shared_state(
        own< shared_state< T > > st) && {
      if (is_ready()) {
        st->set(get_state()->get_value());
        return move(st);
      } else {
        return ext::get< own< shared_state< T > > >(_state);
      }
    }

  public:
    future(future &&) = default;
    future &operator=(future &&) = default;

    bool is_ready() const {
      return apply_visitor(is_ready_visitor{}, _state);
    }
    bool is_exception() const {
      return apply_visitor(is_exception_visitor{}, _state);
    }

    template < class func >
    decltype(auto) then(func &&f) {
      auto *local = ext::get< state< T > >(&_state);
      if (local) {
        return local->set_continuation(forward< func >(f));
      }
      return ext::get< own< shared_state< T > > >(_state)->set_continuation(
          forward< func >(f));
    }

    template < class func >
    decltype(auto) then(mut< core::shard > e, func &&f) {
      auto *local = ext::get< state< T > >(&_state);
      if (local) {
        return local->set_continuation(e, forward< func >(f));
      }
      return ext::get< own< shared_state< T > > >(_state)->set_continuation(
          e, forward< func >(f));
    }
  };

  template <>
  class future< void > : public future<> {
  public:
    using future<>::future;
  };

  template < class T >
  future< T > make_ready_future(T &&value) {
    return future< T >(ready_made, forward< T >(value));
  }
  inline future<> make_ready_future() {
    return future<>(ready_made, meta::null{});
  }
  inline future<> make_ready_future(meta::null) {
    return make_ready_future();
  }
  template < class T >
  future< T > make_ready_future(exception_ptr ex) {
    return future< T >(ready_made, forward< exception_ptr >(ex));
  }
  inline future<> make_ready_future(exception_ptr ex) {
    return future<>(ready_made, forward< exception_ptr >(ex));
  }
  template < class U >
  future< U > make_ready_future(future< U > &&value) {
    return move(value);
  }
}
}
