#include "xi/core/epoll/event_loop.h"
#include "xi/ext/error.h"
#include "xi/core/epoll/detail/event_state_to_short.h"

#include <sys/epoll.h>

namespace xi {
namespace core {
  namespace epoll {
    namespace {
      void epoll_ctl(i32 ep, i32 ctl, i32 desc, i32 flags, void *h) {
        epoll_event event;
        event.data.fd  = desc;
        event.data.ptr = h;
        event.events   = flags;
        if (0 != ::epoll_ctl(ep, ctl, desc, &event)) {
          throw_errno();
        }
      }
    }

    event_loop::event_loop() : _epoll_descriptor(::epoll_create1(EPOLL_CLOEXEC)) {
      if (-1 == _epoll_descriptor) {
        throw_errno();
      }
    }

    event_loop::~event_loop() {
      if (-1 != _epoll_descriptor) {
        ::close(_epoll_descriptor);
      }
    }

    void event_loop::dispatch_events(bool blocking) {
      epoll_event ev[256];
      auto n = ::epoll_wait(_epoll_descriptor, ev, 256, blocking ? -1 : 0);
      for (auto i : range::to(n)) {
        // std::cout << "Event active " << ev[i].data.fd
        //           << " handler " << ev[i].data.ptr
        //           << " flags " << ev[i].events
        //           << std::endl;
        event_callback(ev[i].data.fd, ev[i].events, ev[i].data.ptr);
      }
    }

    void event_loop::event_callback(i32 descriptor,
                                    i32 what,
                                    void *arg) noexcept {
      if (arg) {
        auto *handler = reinterpret_cast< event_handler * >(arg);
        handler->handle(detail::short_to_event_state(what));
      }
    }

    void event_loop::arm_event(i32 desc,
                               event_state state,
                               mut< xi::core::event_handler > h) {
      // std::cout << "Event added " << desc
      //           << " handler " << h
      //           << " flags " << detail::event_state_to_short(state)
      //           << std::endl;
      epoll_ctl(_epoll_descriptor,
                EPOLL_CTL_ADD,
                desc,
                detail::event_state_to_short(state),
                h);
    }
    void event_loop::mod_event(i32 desc,
                               event_state state,
                               mut< xi::core::event_handler > h) {
      epoll_ctl(_epoll_descriptor,
                EPOLL_CTL_MOD,
                desc,
                detail::event_state_to_short(state),
                h);
    }
    void event_loop::del_event(i32 desc) {
      epoll_ctl(_epoll_descriptor, EPOLL_CTL_DEL, desc, 0, nullptr);
    }
  }
}
}
