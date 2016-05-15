#pragma once

#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <iostream>

#include <malloc.h>

namespace xi {
inline namespace ext {
  using ::std::declval;

  using ::std::min;
  using ::std::max;

  using ::std::move;
  using ::std::forward;

  using ::std::function;
  using ::std::bind;

  using ::std::map;
  using ::std::array;
  using ::std::deque;
  using ::std::queue;
  using ::std::vector;
  using ::std::unordered_map;
  using ::std::unordered_set;
  using ::std::initializer_list;
  using ::std::pair;

  using ::std::make_pair;
  using ::std::tie;

  using ::std::distance;
  using ::std::advance;
  using ::std::next;
  using ::std::prev;

  using ::std::begin;
  using ::std::end;

  using ::std::thread;

  using ::std::reference_wrapper;

  using ::std::copy;
  using ::std::find;
  using ::std::find_if;
  using ::std::for_each;

  template < class T >
  decltype(auto) reference(T &t) {
    return ::std::ref(t);
  }

} // inline namespace ext
} // namespace xi
