#pragma once

#include <chrono>

namespace xi {
inline namespace ext {

  using namespace std::literals::chrono_literals;

  using ::std::chrono::duration;
  using ::std::chrono::nanoseconds;
  using ::std::chrono::microseconds;
  using ::std::chrono::milliseconds;
  using ::std::chrono::seconds;
  using ::std::chrono::minutes;
  using ::std::chrono::hours;

  using ::std::chrono::time_point;

  using ::std::chrono::high_resolution_clock;
  using ::std::chrono::steady_clock;

  using ::std::chrono::duration_cast;

} // inline namespace ext
} // namespace xi
