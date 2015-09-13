#pragma once

namespace xi {
inline namespace ext {

  template < int N >
  struct Rank : public Rank< N + 1 > {};
  template <>
  struct Rank< 10 > {};

  static const Rank< 0 > SelectRank{};

} // inline namespace ext
} // namespace xi
