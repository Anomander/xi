#include "xi/core/event.h"

#include <event.h>

namespace xi {
namespace core {
  namespace libevent {
    namespace detail {
      inline short event_state_to_short(event_state state) noexcept {
        short result = 0;
        if (state & kRead) {
          result |= EV_READ;
        }
        if (state & kWrite) {
          result |= EV_WRITE;
        }
        if (state & kTimeout) {
          result |= EV_TIMEOUT;
        }
        result |= EV_ET;

        return result;
      }

      inline event_state short_to_event_state(short state) noexcept {
        event_state result = kNone;
        if (state & EV_READ) {
          result = (event_state)(result | kRead);
        }
        if (state & EV_WRITE) {
          result = (event_state)(result | kWrite);
        }
        if (state & EV_TIMEOUT) {
          result = (event_state)(result | kTimeout);
        }

        return result;
      }
    }
  }
}
}
