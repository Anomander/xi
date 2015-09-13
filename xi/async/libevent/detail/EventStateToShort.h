#include "xi/async/Event.h"

#include <event.h>

namespace xi {
namespace async {
  namespace libevent {
    namespace detail {
      inline short EventStateToShort(EventState state) noexcept {
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
        result |= EV_PERSIST | EV_ET;

        return result;
      }

      inline EventState ShortToEventState(short state) noexcept {
        EventState result = kNone;
        if (state & EV_READ) {
          result = (EventState)(result | kRead);
        }
        if (state & EV_WRITE) {
          result = (EventState)(result | kWrite);
        }
        if (state & EV_TIMEOUT) {
          result = (EventState)(result | kTimeout);
        }

        return result;
      }
    }
  }
}
}
