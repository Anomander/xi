#pragma once

#include "ext/Common.h"

#include <mutex>
#include <shared_mutex>

namespace xi {
inline namespace ext {

  template < class Mutex, class... Args >
  auto makeUniqueLock(Mutex& m, Args&&... args) {
    return ::std::unique_lock< Mutex >(m, forward< Args >(args)...);
  }

  template < class Mutex, class... Args >
  auto makeSharedLock(Mutex& m, Args&&... args) {
    return ::std::shared_lock< Mutex >(m, forward< Args >(args)...);
  }

  template < class Mutex, class... Args >
  auto makeLock(Mutex& m, Args&&... args) {
    return makeUniqueLock(m, forward< Args >(args)...);
  }

} // inline namespace ext
} // namespace xi
