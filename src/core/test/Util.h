#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace test {

  struct ObjectTracker {
    static size_t CREATED;
    static size_t COPIED;
    static size_t MOVED;
    static size_t DESTROYED;

    ObjectTracker() { CREATED++; }
    ObjectTracker(ObjectTracker const &) { COPIED++; }
    ObjectTracker(ObjectTracker &&) { MOVED++; }
    ~ObjectTracker() { DESTROYED++; }

    static void reset() {
      CREATED = 0;
      COPIED = 0;
      MOVED = 0;
      DESTROYED = 0;
    }
  };

  size_t ObjectTracker::CREATED = 0;
  size_t ObjectTracker::COPIED = 0;
  size_t ObjectTracker::MOVED = 0;
  size_t ObjectTracker::DESTROYED = 0;
}
}
