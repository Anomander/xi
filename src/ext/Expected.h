#pragma once

#include "ext/Common.h"
#include "ext/Error.h"
#include "ext/TypeTraits.h"
#include "ext/Variant.h"

namespace xi {
inline namespace ext {

template < class T > class Expected {
public:
  Expected(T val) : _value(move(val)) {}

  Expected(error_code error) : _value(error) {}

  bool hasError() const noexcept { return nullptr != get< error_code >(&_value); }

  operator T const &() const noexcept { return value(); }

  error_code error() const noexcept {
    error_code er;
    if (hasError()) {
      er = *(get< error_code >(&_value));
    }
    return er;
  }

  T const &value() const noexcept {
    if (hasError()) {
      throw system_error(error());
    }
    return get< T >(_value);
  }

private:
  variant< T, error_code > _value;
};

template <> class Expected< void > {
public:
  Expected() = default;

  Expected(error_code error) : _error(error) {}

  bool hasError() const noexcept { return (bool)_error; }

  operator bool() const noexcept { return !hasError(); }

  error_code error() const noexcept { return _error; }

private:
  error_code _error;
};

namespace detail {
template < class T, class F, class R = ::std::result_of_t< F(T) > > struct ExpectedFromReturn {
  using type = Expected< R >;
  static Expected< R > create(Expected< T > const &val, F &f) { return f(val.value()); }
};
template < class F > struct ExpectedFromReturn< void, F, ::std::result_of_t< F() > > {
  using type = Expected< ::std::result_of_t< F() > >;
  static Expected< ::std::result_of_t< F() > > create(Expected< void > const &val, F &f) { return f(); }
};
template < class T, class F > struct ExpectedFromReturn< T, F, void > {
  using type = Expected< void >;
  static Expected< void > create(Expected< T > const &val, F &f) {
    f(val.value());
    return {};
  }
};
}

template < class T, class F > auto fmap(Expected< T > val, F &&f) -> typename detail::ExpectedFromReturn< T, F >::type {
  if (val) {
    return detail::ExpectedFromReturn< T, F >::create(val, f);
  } else {
    return val.error();
  }
}

} // inline namespace ext
} // namespace xi
