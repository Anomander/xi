#pragma once

#include <atomic>

namespace xi {
inline namespace ext {

  using ::std::atomic;
  using ::std::memory_order_acquire;
  using ::std::memory_order_release;
  using ::std::memory_order_relaxed;
  using ::std::memory_order_acq_rel;
  using ::std::memory_order_seq_cst;

  using ::std::atomic_signal_fence;
} // inline namespace ext
} // namespace xi
