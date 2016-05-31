#include "xi/core/netpoller.h"
#include "xi/core/resumable.h"
#include "xi/core/runtime.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>

namespace xi {
namespace core {
  namespace v2 {
    namespace {
      enum { MAX_EVENTS = 1024 };
      thread_local epoll_event EVENTS[MAX_EVENTS];
    }

    class netpoller::impl {
      i32 _epoll     = -1;
      i32 _wakeup_fd = -1;

    public:
      impl();
      void await_readable(resumable* r, i32 fd);
      void await_writable(resumable* r, i32 fd);
      usize poll_into(mut< worker_queue > queue, usize n, bool block);
      void unblock_one();
    };

    netpoller::impl::impl()
        : _epoll(::epoll_create1(EPOLL_CLOEXEC))
        , _wakeup_fd(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {
      epoll_event ev;
      ev.events  = EPOLLIN | EPOLLET;
      ev.data.fd = _wakeup_fd;
      if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, _wakeup_fd, &ev) == -1) {
        ::perror("epoll_ctl: wakeup_fd");
        ::exit(EXIT_FAILURE); // FIXME
      }
    }

    void netpoller::impl::await_readable(resumable* r, i32 fd) {
      epoll_event ev;
      ev.events   = EPOLLIN | EPOLLET | EPOLLONESHOT;
      ev.data.ptr = r;
      auto ret    = epoll_ctl(_epoll, EPOLL_CTL_MOD, fd, &ev);
      if (-1 == ret && errno == ENOENT) {
        ret = epoll_ctl(_epoll, EPOLL_CTL_ADD, fd, &ev);
        if (-1 == ret) {
          if (errno == EEXIST) {
            return;
          }
          perror("epoll_ctl: await_readable");
          exit(EXIT_FAILURE);
        }
      }
    }

    void netpoller::impl::await_writable(resumable* r, i32 fd) {
      epoll_event ev;
      ev.events   = EPOLLOUT | EPOLLET | EPOLLONESHOT;
      ev.data.ptr = r;
      auto ret    = epoll_ctl(_epoll, EPOLL_CTL_MOD, fd, &ev);
      if (-1 == ret && errno == ENOENT) {
        ret = epoll_ctl(_epoll, EPOLL_CTL_ADD, fd, &ev);
        if (-1 == ret) {
          if (errno == EEXIST) {
            return;
          }
          perror("epoll_ctl: await_writable");
          exit(EXIT_FAILURE);
        }
      }
    }

    usize netpoller::impl::poll_into(mut< worker_queue > q,
                                     usize n,
                                     bool block) {
      auto& events = EVENTS;
      i32 cnt      = ::epoll_wait(
          _epoll, events, min< usize >(n, MAX_EVENTS), block ? -1 : 0);
      if (cnt == -1 && errno == EINTR) {
        return 0;
      }
      assert(cnt >= 0);
      for (i32 i = 0; i < cnt; ++i) {
        if (XI_UNLIKELY(events[i].data.fd == _wakeup_fd)) {
          u64 val;
          ::eventfd_read(_wakeup_fd, &val);
          continue;
        }
        auto r = reinterpret_cast< resumable* >(events[i].data.ptr);
        q->enqueue(own< resumable >{r});
      }
      return cnt;
    }

    void netpoller::impl::unblock_one() {
      ::eventfd_write(_wakeup_fd, 1);
    }

    void netpoller::start() {
      _impl = make_shared< impl >();
    }

    void netpoller::stop() {
      _impl.reset();
    }

    void netpoller::await_readable(i32 fd, own< resumable > r) {
      assert(_impl);
      _impl->await_readable(r.release(), fd);
    }

    void netpoller::await_writable(i32 fd, own< resumable > r) {
      assert(_impl);
      _impl->await_writable(r.release(), fd);
    }

    usize netpoller::poll_into(mut< worker_queue > q, usize n) {
      assert(_impl);
      return _impl->poll_into(q, n, false);
    }

    usize netpoller::blocking_poll_into(mut< worker_queue > q, usize n) {
      assert(_impl);
      return _impl->poll_into(q, n, true);
    }

    void netpoller::unblock_one() {
      assert(_impl);
      return _impl->unblock_one();
    }
  }
}
}
