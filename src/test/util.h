#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace test {

  struct object_tracker {
    static size_t CREATED;
    static size_t COPIED;
    static size_t MOVED;
    static size_t DESTROYED;

    object_tracker() { CREATED++; }
    object_tracker(object_tracker const &) { COPIED++; }
    object_tracker(object_tracker &&) { MOVED++; }
    ~object_tracker() { DESTROYED++; }

    static void reset() {
      CREATED = 0;
      COPIED = 0;
      MOVED = 0;
      DESTROYED = 0;
    }
  };

  size_t object_tracker::CREATED = 0;
  size_t object_tracker::COPIED = 0;
  size_t object_tracker::MOVED = 0;
  size_t object_tracker::DESTROYED = 0;
}
}
