#pragma once

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>

namespace xi {
inline namespace ext {
  namespace lockfree {
    using ::boost::lockfree::spsc_queue;
    using ::boost::lockfree::capacity;

    using ::boost::lockfree::queue;
  } // namespace lockfree
} // inline namespace ext
} // namespace xi
