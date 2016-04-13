#include "xi/core/event.h"

#include <sys/epoll.h>

namespace xi {
namespace core {
  namespace epoll {
    namespace detail {
      inline i32 event_state_to_short(event_state state) noexcept {
        i32 result = 0;
        if (state & kRead) {
          result |= EPOLLIN;
        }
        if (state & kWrite) {
          result |= EPOLLOUT;
        }
        // result |= EPOLLET;
        result |= EPOLLRDHUP;

        return result;
      }

      inline event_state short_to_event_state(i32 state) noexcept {
        event_state result = kNone;
        if (state & EPOLLOUT) {
          result = (event_state)(result | kWrite);
        }
        if (state & EPOLLIN) {
          result = (event_state)(result | kRead);
        }
        if (state & EPOLLRDHUP) {
          result = (event_state)(result | kClose);
        }
        if (state & (EPOLLHUP | EPOLLERR)) {
          result = (event_state)(result | kError);
        }

        return result;
      }
    }
  }
}
}
