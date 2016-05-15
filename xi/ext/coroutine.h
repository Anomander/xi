#pragma once

#include <boost/coroutine/all.hpp>

namespace xi {
inline namespace ext {
  using ::boost::coroutines::symmetric_coroutine;
  using ::boost::coroutines::attributes;
  using ::boost::coroutines::standard_stack_allocator;
}
}
