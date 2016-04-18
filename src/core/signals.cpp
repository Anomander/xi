#include "xi/core/signals.h"
#include "xi/ext/number.h"
#include "xi/core/shard.h"
#include <signal.h>

namespace xi {
namespace core {
  shard::signals::signals() {
    sigset_t sigs;
    ::sigfillset(&sigs);
    for (auto sig : {SIGHUP,
                     SIGQUIT,
                     SIGILL,
                     SIGABRT,
                     SIGFPE,
                     SIGSEGV,
                     SIGALRM,
                     SIGCONT,
                     SIGSTOP,
                     SIGTSTP,
                     SIGTTIN,
                     SIGTTOU}) {
      ::sigdelset(&sigs, sig);
    }
    ::pthread_sigmask(SIG_BLOCK, &sigs, nullptr);
  }

  shard::signals::~signals() {
    sigset_t sigs;
    ::sigfillset(&sigs);
    ::pthread_sigmask(SIG_BLOCK, &sigs, nullptr);
  }

  void shard::signals::handle(i32 signo, function< void() > handler) {
    _handlers[signo] = move(handler);
    struct sigaction sa;
    sa.sa_sigaction = [](int signo, siginfo_t*, void*) {
      xi::shard()->_signals->_pending_signals.fetch_or(
          1ull << signo, std::memory_order_relaxed);
    };
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    if (::sigaction(signo, &sa, nullptr) < 0) {
      throw_errno();
    }
    sigset_t mask;
    ::sigemptyset(&mask);
    ::sigaddset(&mask, signo);
    if (::pthread_sigmask(SIG_UNBLOCK, &mask, NULL) < 0) {
      throw_errno();
    }
    std::cout << "Registering " << signo << std::endl;
  }

  u32 shard::signals::poll() noexcept {
    u64 signals = _pending_signals.load(std::memory_order_relaxed);
    if (signals) {
      _pending_signals.fetch_and(~signals, std::memory_order_relaxed);
      while (signals > 0) {
        std::cout << "Remaining " << signals << std::endl;
        auto i = count_trailing_zeroes(signals);
        std::cout << "Handling " << i << std::endl;
        _handlers.at(i)();
        signals >>= (i + 1);
      }
    }
    return signals;
  }
}
}
