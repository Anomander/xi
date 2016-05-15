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

      inline u16 short_to_event_state(i32 state) noexcept {
        u16 result = kNone;
        if (state & EPOLLOUT) {
          result |= kWrite;
        }
        if (state & EPOLLIN) {
          result |= kRead;
        }
        if (state & EPOLLRDHUP) {
          result |= kClose;
        }
        if (state & (EPOLLHUP | EPOLLERR)) {
          result |= kError;
        }

        return result;
      }
    }
  }
}
}
