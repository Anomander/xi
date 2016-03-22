#pragma once

#include "xi/ext/common.h"

#include <mutex>
#include <shared_mutex>

namespace xi {
inline namespace ext {

  template < class mutex, class... A >
  auto make_unique_lock(mutex &m, A &&... args) {
    return ::std::unique_lock< mutex >(m, forward< args >(args)...);
  }

  template < class mutex, class... A >
  auto make_shared_lock(mutex &m, A &&... args) {
    return ::std::shared_lock< mutex >(m, forward< args >(args)...);
  }

  template < class mutex, class... A >
  auto make_lock(mutex &m, A &&... args) {
    return make_unique_lock(m, forward< args >(args)...);
  }

} // inline namespace ext
} // namespace xi
