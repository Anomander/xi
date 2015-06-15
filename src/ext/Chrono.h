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

} // inline namespace ext
} // namespace xi
