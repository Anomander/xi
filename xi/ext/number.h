#pragma once

#include <algorithm>

namespace xi {
inline namespace ext {

  using ::std::min;
  using ::std::max;
  using ::std::numeric_limits;

  inline constexpr auto count_leading_zeroes(unsigned x) {
    return __builtin_clz(x);
  }

  inline constexpr auto count_leading_zeroes(unsigned long x) {
    return __builtin_clzl(x);
  }

  inline constexpr auto count_leading_zeroes(unsigned long long x) {
    return __builtin_clzll(x);
  }

  inline constexpr auto count_trailing_zeroes(unsigned x) {
    return __builtin_ctz(x);
  }

  inline constexpr auto count_trailing_zeroes(unsigned long x) {
    return __builtin_ctzl(x);
  }

  inline constexpr auto count_trailing_zeroes(unsigned long long x) {
    return __builtin_ctzll(x);
  }

  template < class T >
  class distinct_numeric_type {
    static_assert(::std::is_arithmetic< T >::value,
                  "Type T must be a numeric type.");
    T _value;

  public:
    using underlying_type = T;

    explicit constexpr distinct_numeric_type(T v) : _value(v) {
    }
    constexpr distinct_numeric_type()                              = default;
    constexpr distinct_numeric_type(distinct_numeric_type const &) = default;
    constexpr distinct_numeric_type &operator=(distinct_numeric_type const &) =
        default;
    distinct_numeric_type &operator=(T const &v) {
      _value = v;
    }

    constexpr bool operator==(distinct_numeric_type const &other) const {
      return _value == other._value;
    }
    constexpr bool operator!=(distinct_numeric_type const &other) const {
      return _value != other._value;
    }
    constexpr bool operator<(distinct_numeric_type const &other) const {
      return _value < other._value;
    }
    constexpr bool operator<=(distinct_numeric_type const &other) const {
      return _value <= other._value;
    }
    constexpr bool operator>(distinct_numeric_type const &other) const {
      return _value > other._value;
    }
    constexpr bool operator>=(distinct_numeric_type const &other) const {
      return _value >= other._value;
    }
    distinct_numeric_type &operator+(distinct_numeric_type const &other) {
      _value + other._value;
      return *this;
    }
    distinct_numeric_type &operator-(distinct_numeric_type const &other) {
      _value - other._value;
      return *this;
    }
    distinct_numeric_type &operator+=(distinct_numeric_type const &other) {
      _value += other._value;
      return *this;
    }
    distinct_numeric_type &operator-=(distinct_numeric_type const &other) {
      _value -= other._value;
      return *this;
    }
    constexpr operator T() const {
      return _value;
    }
  };
} // inline namespace ext
} // namespace xi

namespace std {
template < typename T >
struct numeric_limits< xi::ext::distinct_numeric_type< T > >
    : public numeric_limits< T > {};
}
