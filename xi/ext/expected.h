#pragma once

#include "xi/ext/common.h"
#include "xi/ext/error.h"
#include "xi/ext/type_traits.h"
#include "xi/ext/variant.h"

namespace xi {
inline namespace ext {

  template < class T >
  class expected {
  public:
    expected(T val) : _value(move(val)) {
    }

    expected(error_code error) : _value(error) {
    }

    bool has_error() const noexcept {
      return nullptr != get< error_code >(&_value);
    }

    operator T const &() const noexcept {
      return value();
    }

    error_code error() const noexcept {
      error_code er;
      if (has_error()) {
        er = *(get< error_code >(&_value));
      }
      return er;
    }

    T const &value() const noexcept {
      if (has_error()) {
        throw system_error(error());
      }
      return get< T >(_value);
    }

    T &&move_value() noexcept {
      if (has_error()) {
        throw system_error(error());
      }
      return move(get< T >(_value));
    }

  private:
    variant< T, error_code > _value;
  };

  template <>
  class expected< void > {
  public:
    expected() = default;

    expected(error_code error) : _error(error) {
    }

    template < class T >
    expected(expected< T > &&other)
        : _error(other.has_error() ? other.error() : error_code()) {
    }

    bool has_error() const noexcept {
      return (bool)_error;
    }

    operator bool() const noexcept {
      return !has_error();
    }

    error_code error() const noexcept {
      return _error;
    }

  private:
    error_code _error;
  };

  namespace detail {
    template < class T, class F, class R = result_of_t< F(T) > >
    struct expected_from_return {
      using type = expected< R >;
      static expected< R > create(expected< T > const &val, F &f) {
        return f(val.value());
      }
    };
    template < class F >
    struct expected_from_return< void, F, result_of_t< F() > > {
      using type = expected< result_of_t< F() > >;
      static expected< result_of_t< F() > > create(expected< void > const &,
                                                   F &f) {
        return f();
      }
    };
    template < class T, class F >
    struct expected_from_return< T, F, void > {
      using type = expected< void >;
      static expected< void > create(expected< T > const &val, F &f) {
        f(val.value());
        return {};
      }
    };
  }

  template < class F, class... A >
  auto posix_to_expected(F f, A &&... args) {
    auto ret       = f(forward< A >(args)...);
    using return_t = expected< decltype(ret) >;
    if (-1 == ret) {
      return return_t{error_from_errno()};
    }
    return return_t{ret};
  }

  template < class T, class F >
  auto fmap(expected< T > val, F &&f) ->
      typename detail::expected_from_return< T, F >::type {
    if (val) {
      return detail::expected_from_return< T, F >::create(val, f);
    } else {
      return val.error();
    }
  }

} // inline namespace ext
} // namespace xi
