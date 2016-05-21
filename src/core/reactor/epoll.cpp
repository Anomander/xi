#include "xi/core/reactor/epoll.h"
#include "xi/ext/configure.h"
#include "xi/core/execution_context.h"
#include "xi/core/runtime.h"

#include <signal.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>

namespace xi {
namespace core {
  namespace reactor {
    namespace {
      atomic< u64 > SIGNALS;
      const auto WAKEUP_SIGNAL = SIGRTMIN;
      const auto QUOTA_SIGNAL  = WAKEUP_SIGNAL + 1;

      auto make_full_sigset_mask() {
        sigset_t set;
        ::sigfillset(&set);
        return set;
      }

      auto make_empty_sigset_mask() {
        sigset_t set;
        ::sigemptyset(&set);
        return set;
      }

      auto make_sigset_mask(int signo) {
        sigset_t set;
        ::sigemptyset(&set);
        ::sigaddset(&set, signo);
        return set;
      }

      void block_all_signals() {
        sigset_t mask;
        ::sigfillset(&mask);
        ::sigdelset(&mask, SIGINT);
        ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
      }

      void unblock_all_signals() {
        sigset_t mask;
        ::sigfillset(&mask);
        ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
      }

      void yield_current_task(int signo) {
        // printf("Task quota reached.\n");
        xi::core::runtime.local_worker().current_resumable()->yield(
            resumable::resume_later);
      }

      void action(int signo, siginfo_t* siginfo, void* ignore) {
        printf("Signal %d\n", signo);
        if (signo == QUOTA_SIGNAL) {
        }
        SIGNALS.fetch_or(1ull << signo, memory_order_relaxed);
      }

      void unblock_signal(i32 signo) {
        sigset_t mask;
        ::sigemptyset(&mask);
        ::sigaddset(&mask, signo);
        ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
      }

      void handle_signal(i32 signo) {
        struct sigaction sa;
        sa.sa_sigaction = action;
        sa.sa_mask      = make_empty_sigset_mask();
        sa.sa_flags     = SA_SIGINFO | SA_RESTART;
        auto r          = ::sigaction(signo, &sa, nullptr);
        assert(r == 0);
        auto mask = make_sigset_mask(signo);
        r         = ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
        assert(r == 0);
      }
    }

    epoll::epoll()
        : _epoll(::epoll_create1(EPOLL_CLOEXEC))
        , _wakeup_fd(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK))
        , _timer_fd(
              ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK))
        , _thread_id(::pthread_self()) {
      assert(_epoll >= 0);
      epoll_event ev;
      ev.events  = EPOLLIN | EPOLLET;
      ev.data.fd = _wakeup_fd;
      if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, _wakeup_fd, &ev) == -1) {
        ::perror("epoll_ctl: wakeup_fd");
        ::exit(EXIT_FAILURE); // FIXME
      }
      ev.data.fd = _timer_fd;
      if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, _timer_fd, &ev) == -1) {
        ::perror("epoll_ctl: timer_fd");
        ::exit(EXIT_FAILURE); // FIXME
      }
      sigevent sev;
      sev.sigev_notify   = SIGEV_THREAD_ID;
      sev._sigev_un._tid = syscall(SYS_gettid);
      sev.sigev_signo    = QUOTA_SIGNAL;
      auto ret = ::timer_create(CLOCK_THREAD_CPUTIME_ID, &sev, &_quota_timer);
      perror("timer_create");
      assert(0 <= ret);
      block_all_signals();
      handle_signal(WAKEUP_SIGNAL);
      handle_signal(QUOTA_SIGNAL);

      struct sigaction sa_task_quota = {};
      sa_task_quota.sa_handler       = &yield_current_task;
      auto r = sigaction(QUOTA_SIGNAL, &sa_task_quota, nullptr);
      assert(r == 0);
    }

    epoll::~epoll() {
      assert(_epoll >= 0);
      ::close(_epoll);
      ::timer_delete(_quota_timer);
      unblock_all_signals();
    }

    void epoll::poll_for(nanoseconds ns) {
      constexpr u16 MAX_EVENTS = 1024;
      epoll_event events[MAX_EVENTS];
      // sigset_t block_all, active_sigmask;
      // auto mask = make_sigset_mask(WAKEUP_SIGNAL);
      auto mask = make_empty_sigset_mask();
      // sigfillset(&block_all);
      // sigdelset(&block_all, WAKEUP_SIGNAL);
      // ::pthread_sigmask(SIG_SETMASK, &block_all, &active_sigmask);
      auto timeout = 0;
      if (ns > 0ns) {
        auto tv_nsec         = ns.count() % 1000'000'000;
        auto tv_sec          = ns.count() / 1000'000'000;
        itimerspec its       = {};
        its.it_value.tv_nsec = tv_nsec;
        its.it_value.tv_sec  = tv_sec;
        // its.it_interval      = its.it_value;
        its.it_interval = {};
        auto r          = timerfd_settime(_timer_fd, 0, &its, nullptr);
        assert(r >= 0);
        timeout = -1;
      }
      auto n = ::epoll_pwait(_epoll, events, MAX_EVENTS, timeout, &mask);
      // ::pthread_sigmask(SIG_SETMASK, &active_sigmask, nullptr);

      if (n == -1 && errno == EINTR) {
        return;
      }

      assert(n >= 0);
      for (i32 i = 0; i < n; ++i) {
        if (XI_UNLIKELY(events[i].data.fd == _wakeup_fd)) {
          u64 val;
          ::eventfd_read(_wakeup_fd, &val);
        } else if (XI_LIKELY(events[i].data.fd == _timer_fd)) {
          // do nothing
        } else {
          // printf("Thread %p Handling resumable %p\n", pthread_self(), events[i].data.ptr);
          reinterpret_cast< resumable* >(events[i].data.ptr)->unblock();
        }
      }
    }

    void epoll::attach_pollable(resumable* r, i32 fd) {
      epoll_event ev;
      ev.events   = EPOLLERR;
      ev.data.fd  = fd;
      ev.data.ptr = r;
      if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, fd, &ev) == -1) {
        ::perror("epoll_ctl: attach_pollable");
        ::exit(EXIT_FAILURE); // FIXME
      }
    }

    void epoll::detach_pollable(resumable*, i32 fd) {
      if (::epoll_ctl(_epoll, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        printf("Detaching %d failed.\n", fd);
        ::perror("epoll_ctl: detach_pollable");
        ::exit(EXIT_FAILURE); // FIXME
      }
    }

    void epoll::await_signal(resumable*, i32) {
      // TODO: Implement
    }

    void epoll::await_readable(resumable* r, i32 fd) {
      // printf("Awaiting readability for resumable %p\n", r);
      epoll_event ev;
      ev.events = EPOLLIN | EPOLLONESHOT;
      // ev.data.fd  = fd;
      ev.data.ptr = r;
      if (epoll_ctl(_epoll, EPOLL_CTL_MOD, fd, &ev) == -1) {
        perror("epoll_ctl: await_writable");
        exit(EXIT_FAILURE);
      }
      r->block();
    }

    void epoll::await_writable(resumable* r, i32 fd) {
      epoll_event ev;
      ev.events = EPOLLOUT | EPOLLONESHOT;
      // ev.data.fd  = fd;
      ev.data.ptr = r;
      if (epoll_ctl(_epoll, EPOLL_CTL_MOD, fd, &ev) == -1) {
        perror("epoll_ctl: await_writable");
        exit(EXIT_FAILURE);
      }
      r->block();
    }

    void epoll::maybe_wakeup() {
      ::eventfd_write(_wakeup_fd, 1);
      // pthread_kill(_thread_id, WAKEUP_SIGNAL);
    }
    void epoll::begin_task_quota_monitor(nanoseconds ns) {
      auto tv_nsec         = ns.count() % 1000'000'000;
      auto tv_sec          = ns.count() / 1000'000'000;
      itimerspec its       = {};
      its.it_value.tv_nsec = tv_nsec;
      its.it_value.tv_sec  = tv_sec;
      // its.it_interval      = its.it_value;
      its.it_interval = {};
      auto r          = timer_settime(_quota_timer, 0, &its, nullptr);
      assert(r >= 0);
    }
    void epoll::end_task_quota_monitor() {
      itimerspec its = {};
      auto r         = timer_settime(_quota_timer, 0, &its, nullptr);
      assert(r >= 0);
    }
  }
}
}
