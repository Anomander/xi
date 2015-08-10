#pragma once

#include <map>
#include <array>
#include <deque>
#include <tuple>
#include <memory>
#include <thread>
#include <vector>
#include <functional>
#include <unordered_map>

namespace xi {
inline namespace ext {
  using ::std::declval;

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
  using ::std::initializer_list;

  using ::std::thread;

  using ::std::reference_wrapper;

  template < class T >
  decltype(auto) reference(T& t) {
    return ::std::ref(t);
  }

} // inline namespace ext
} // namespace xi
