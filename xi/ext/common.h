#pragma once

#include <map>
#include <array>
#include <deque>
#include <tuple>
#include <queue>
#include <memory>
#include <thread>
#include <vector>
#include <functional>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

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
  using ::std::queue;
  using ::std::vector;
  using ::std::unordered_map;
  using ::std::unordered_set;
  using ::std::initializer_list;

  using ::std::distance;

  using ::std::begin;
  using ::std::end;

  using ::std::thread;

  using ::std::reference_wrapper;

  using ::std::copy;

  template < class T > decltype(auto) reference(T &t) { return ::std::ref(t); }

} // inline namespace ext
} // namespace xi
