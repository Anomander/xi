#pragma once

#include "xi/ext/common.h"

#include <mutex>
#include <shared_mutex>

namespace xi {
inline namespace ext {

  using ::std::mutex;
  using ::std::condition_variable;

  template < class Mutex, class... A >
  auto make_unique_lock(Mutex &m, A &&... args) {
    return ::std::unique_lock< Mutex >(m, forward< args >(args)...);
  }

  template < class Mutex, class... A >
  auto make_shared_lock(Mutex &m, A &&... args) {
    return ::std::shared_lock< Mutex >(m, forward< args >(args)...);
  }

  template < class Mutex, class... A >
  auto make_lock(Mutex &m, A &&... args) {
    return make_unique_lock(m, forward< args >(args)...);
  }

} // inline namespace ext
} // namespace xi
