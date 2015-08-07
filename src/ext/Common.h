#pragma once

#include <map>
#include <array>
#include <deque>
#include <tuple>
#include <atomic>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

#include <boost/regex.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace xi {
inline namespace ext {

  using ::std::min;
  using ::std::max;

  using ::std::move;
  using ::std::forward;

  using ::std::function;

  using ::std::map;
  using ::std::array;
  using ::std::deque;
  using ::std::vector;
  using ::std::unordered_map;

  using ::std::atomic;

  using ::boost::regex;
  using ::boost::cmatch; /// match_results <char*>

  template < class T >
  decltype(auto) reference(T& t) {
    return ::std::ref(t);
  }

  using ::boost::multiprecision::uint128_t;
  using ::boost::multiprecision::uint256_t;
  using ::boost::multiprecision::uint512_t;
  using ::boost::multiprecision::uint1024_t;

} // inline namespace ext
} // namespace xi
