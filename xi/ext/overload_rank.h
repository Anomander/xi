#pragma once

namespace xi {
inline namespace ext {

  template < int N >
  struct rank : public rank< N + 1 > {};
  template <>
  struct rank< 10 > {};

  static const rank< 0 > select_rank{};

} // inline namespace ext
} // namespace xi
