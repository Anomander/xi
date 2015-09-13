#pragma once

#include <atomic>

namespace xi {
inline namespace ext {

  using ::std::atomic;
  using ::std::memory_order_acquire;
  using ::std::memory_order_release;
  using ::std::memory_order_relaxed;

} // inline namespace ext
} // namespace xi
