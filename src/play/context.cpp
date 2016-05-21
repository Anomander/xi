#include <array>
#include <deque>
#include <functional>
#include <iostream>

#include <execinfo.h>
#include <string.h>

#include "xi/ext/configure.h"
#include "xi/core/channel.h"
#include "xi/core/coordinator.h"
#include "xi/core/generic_resumable.h"
#include "xi/core/policy/worker_isolation.h"
#include "xi/core/reactor/abstract_reactor.h"
#include "xi/core/runtime.h"
#include "xi/core/worker.h"
#include "xi/io/error.h"
#include "xi/util/result.h"

#include <boost/context/all.hpp>
#include <boost/coroutine/all.hpp>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace boost::context;
using namespace boost::coroutines;
using namespace xi;

// void
// print_stack(void) {
//   constexpr int BT_BUF_SIZE = 256;
//   int j, nptrs;
//   void* buffer[BT_BUF_SIZE];
//   char** strings;

//   nptrs = backtrace(buffer, BT_BUF_SIZE);
//   printf("backtrace() returned %d addresses\n", nptrs);

//   /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
//      would produce similar output to the following: */

//   strings = backtrace_symbols(buffer, nptrs);
//   if (strings == NULL) {
//     perror("backtrace_symbols");
//     exit(EXIT_FAILURE);
//   }

//   for (j = 0; j < nptrs; j++)
//     printf("%s\n", strings[j]);

//   free(strings);
// }

// void*
// operator new(std::size_t count) {
//   printf("Allocating %u\n", count);
//   print_stack();
//   auto p = std::malloc(count);
//   printf("++++++ Allocated %u @ %p\n", count, p);
//   return p;
// }
// void
// operator delete(void* p) {
//   std::free(p);
//   printf("------ Deallocated %p\n", p);
// }

class resumable;

struct epoll {
  int _desc;

public:
  epoll() : _desc(::epoll_create(10)) {
  }
  void poll(bool blocking);

  void add(int fd) {
    // printf("%s %d\n", __PRETTY_FUNCTION__, fd);
    epoll_event ev;
    ev.events  = EPOLLERR;
    ev.data.fd = fd;
    if (epoll_ctl(_desc, EPOLL_CTL_ADD, fd, &ev) == -1) {
      perror("epoll_ctl: listen_sock");
      exit(EXIT_FAILURE);
    }
  }
  void del(int fd) {
    // printf("%s %d\n", __PRETTY_FUNCTION__, fd);
    if (epoll_ctl(_desc, EPOLL_CTL_DEL, fd, nullptr) == -1) {
      perror("epoll_ctl: listen_sock");
      exit(EXIT_FAILURE);
    }
  }

  void wait_readable(int fd);

  void wait_writable(int fd, resumable* r = nullptr);
};

static thread_local epoll* EPOLL = new epoll;

class worker;

xi::core::worker< core::policy::worker_isolation > W;

class resumable : public intrusive::list_base_hook<
                      intrusive::link_mode< intrusive::normal_link > > {
public:
  enum resume_result {
    blocked,
    resume_later,
    done,
  };

  using coro_t  = symmetric_coroutine< void >::call_type;
  using yield_t = symmetric_coroutine< void >::yield_type;

  resumable()
      : _coro(
            [this](yield_t& y) {
              _yield = &y;
              call();
              _result = done;
            },
            attributes(),
            standard_stack_allocator()) {
    printf("%s %p\n", __PRETTY_FUNCTION__, this);
  }

  virtual ~resumable() {
    printf("%s %p\n", __PRETTY_FUNCTION__, this);
  }
  XI_CLASS_DEFAULTS(resumable, no_move, no_copy);

  resume_result resume() {
    _coro();
    return _result;
  }

  void block();

  void unblock();

protected:
  void yield(resume_result result = resume_later) {
    _result = result;
    (*_yield)();
  }

  virtual void call() = 0;

private:
  coro_t _coro;
  yield_t* _yield       = nullptr;
  resume_result _result = resume_later;
  worker* _unblock_on   = nullptr;
};

class worker {
  deque< resumable* > _ready_queue;
  resumable* _active_task = nullptr;

public:
  void push(resumable* r) {
    _ready_queue.push_back(r);
  }

  resumable* active_task() {
    return _active_task;
  }

  void run();
};

static thread_local worker* WORKER = new worker;

void
resumable::block() {
  _unblock_on = WORKER;
  yield(blocked);
}

void
resumable::unblock() {
  _unblock_on->push(this);
}

void
worker::run() {
  for (;;) {
    EPOLL->poll(_ready_queue.empty());
    if (_ready_queue.empty()) {
      continue;
    }
    _active_task = _ready_queue.front();
    _ready_queue.pop_front();

    switch (_active_task->resume()) {
      case resumable::done:
        delete _active_task;
        break;
      case resumable::blocked:
        break;
      case resumable::resume_later:
        _ready_queue.push_back(_active_task);
        break;
    }
    _active_task = nullptr;
  }
}

void
epoll::wait_readable(int fd) {
  // printf("%s %d\n", __PRETTY_FUNCTION__, fd);
  auto task = WORKER->active_task();

  epoll_event ev;
  ev.events   = EPOLLIN | EPOLLONESHOT;
  ev.data.fd  = fd;
  ev.data.ptr = task;
  if (epoll_ctl(_desc, EPOLL_CTL_MOD, fd, &ev) == -1) {
    perror("epoll_ctl: wait_readable");
    exit(EXIT_FAILURE);
  }
  task->block();
}

void
epoll::wait_writable(int fd, resumable* r) {
  // printf("%s %d\n", __PRETTY_FUNCTION__, fd);
  epoll_event ev;
  ev.events   = EPOLLOUT | EPOLLONESHOT;
  ev.data.fd  = fd;
  auto task   = (r ? r : WORKER->active_task());
  ev.data.ptr = task;
  if (epoll_ctl(_desc, EPOLL_CTL_MOD, fd, &ev) == -1) {
    perror("epoll_ctl: wait_writable");
    exit(EXIT_FAILURE);
  }
  task->block();
}

void
epoll::poll(bool blocking) {
  epoll_event events[1024];
  int n = ::epoll_wait(_desc, events, 1024, blocking ? -1 : 0);
  // printf("%s %d %d\n", __PRETTY_FUNCTION__, n, errno);
  for (int i = 0; i < n; ++i) {
    reinterpret_cast< resumable* >(events[i].data.ptr)->unblock();
  }
}

class socket {
  int _descriptor;
  epoll* _epoll;

public:
  socket(int s) : _descriptor(s), _epoll(EPOLL) {
    _epoll->add(_descriptor);
  }
  ~socket() {
    _epoll->del(_descriptor);
  }

  result< int > read_some(char* data, size_t size) {
    int r = -1;
    for (;;) {
      r = ::recv(_descriptor, data, size, MSG_DONTWAIT);
      // printf("%s %d\n", __PRETTY_FUNCTION__, r);
      if (r < 0) {
        if (errno == EINTR) {
          continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          _epoll->wait_readable(_descriptor);
          continue;
        }
        return result_from_errno< int >();
      }
      if (r == 0) {
        return {xi::io::error::kEOF};
      }
      break;
    }
    return ok(r);
  }

  result< int > write_some(char* data, size_t size) {
    int r = -1;
    for (;;) {
      // printf("%s\n", __PRETTY_FUNCTION__);
      r = ::send(_descriptor, data, size, MSG_DONTWAIT | MSG_NOSIGNAL);
      if (r < 0) {
        if (errno == EINTR) {
          continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          _epoll->wait_writable(_descriptor);
          continue;
        }
        return result_from_errno< int >();
      }
      break;
    }
    return ok(r);
  }

  result< size_t > read_exactly(char* data, size_t size) {
    auto* p = data;
    auto s  = 0ull;
    while (s < size) {
      auto ret = read_some(p, size - s);
      if (ret.is_error()) {
        return move(ret);
      }
      auto r = ret.unwrap();
      p += r;
      s += r;
    }
    return ok(s);
  }

  result< size_t > write_exactly(char* data, size_t size) {
    auto* p = data;
    auto s  = 0ull;
    while (s < size) {
      auto ret = write_some(p, size - s);
      if (ret.is_error()) {
        return move(ret);
      }
      auto r = ret.unwrap();
      p += r;
      s += r;
    }
    return ok(s);
  }
};

struct channel : public resumable {
  channel(int sock) : _socket(sock) {
  }

  void call() override {
    std::vector< char > data(4 << 12);
    for (;;) {
      auto r = _socket.read_some(data.data(), data.size()).then([&](int size) {
        return _socket.write_exactly(data.data(), size);
      });
      if (r.is_error()) {
        break;
      }
    }
  }

private:
  class socket _socket;
};

struct acceptor : public resumable {
  acceptor()
      : _socket(::socket(
            AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)) {
    struct sockaddr_in addr;
    ::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(19999);
    addr.sin_addr.s_addr = INADDR_ANY;
    int reuse            = 1;
    setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (bind(_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      perror("bind");
      exit(EXIT_FAILURE);
    }
    if (listen(_socket, 5) < 0) {
      perror("listen");
      exit(EXIT_FAILURE);
    }
    EPOLL->add(_socket);
  }

  void call() override {
    for (;;) {
      EPOLL->wait_readable(_socket);
      sockaddr_in* addr;
      socklen_t len = sizeof(addr);
      for (;;) {
        int sock = ::accept4(
            _socket, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (sock < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
          }
          EPOLL->del(_socket);
          perror("accept");
          return;
        }
        printf("Connection! %d\n", sock);
        WORKER->push(new channel(sock));
      }
    }
  }

private:
  int _socket;
};

namespace xi {
class resumable_fd {
  i32 _fd;
  core::abstract_reactor* _reactor;
  core::resumable* _process;

protected:
  resumable_fd(i32 fd, core::resumable* p) : _fd(fd), _process(p) {
    _reactor = core::runtime.local_worker().attach_pollable(_process, _fd);
  }

  ~resumable_fd() {
    _reactor->detach_pollable(_process, _fd);
    ::close(_fd);
  }

  i32 fd() const {
    return _fd;
  }

  void await_readable() {
    _reactor->await_readable(_process, _fd);
  }

  void await_writable() {
    _reactor->await_writable(_process, _fd);
  }

  result< int > read_some(char* data, size_t size) {
    int r = -1;
    for (;;) {
      r = ::recv(_fd, data, size, MSG_DONTWAIT);
      // printf("%s %d\n", __PRETTY_FUNCTION__, r);
      if (r < 0) {
        if (errno == EINTR) {
          continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          await_readable();
          continue;
        }
        return result_from_errno< int >();
      }
      if (r == 0) {
        return {xi::io::error::kEOF};
      }
      break;
    }
    return ok(r);
  }

  result< int > write_some(char* data, size_t size) {
    int r = -1;
    for (;;) {
      // printf("%s\n", __PRETTY_FUNCTION__);
      r = ::send(_fd, data, size, MSG_DONTWAIT | MSG_NOSIGNAL);
      if (r < 0) {
        if (errno == EINTR) {
          continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          await_writable();
          continue;
        }
        return result_from_errno< int >();
      }
      break;
    }
    return ok(r);
  }

  result< size_t > write_exactly(char* data, size_t size) {
    auto* p = data;
    auto s  = 0ull;
    while (s < size) {
      auto ret = write_some(p, size - s);
      if (ret.is_error()) {
        return move(ret);
      }
      auto r = ret.unwrap();
      p += r;
      s += r;
    }
    return ok(s);
  }
};
}

struct channel2 : public resumable_fd, public core::generic_resumable {
  channel2(int sock) : resumable_fd(sock, this) {
  }

  void call() override {
    std::vector< char > data(4 << 12);
    for (;;) {
      auto r = read_some(data.data(), data.size()).then([&](int size) {
        return write_exactly(data.data(), size);
      });
      if (r.is_error()) {
        break;
      }
    }
  }
};

namespace xi {
namespace core {
  template < class F,
             class... Args,
             XI_REQUIRE_DECL(is_base_of< resumable, F >) >
  void spawn(Args&&... args) {
    auto& coordinator = runtime.coordinator();
    auto maker        = [](Args&&... args) {
      runtime.local_worker().spawn_resumable< F >(forward< Args >(args)...);
    };
    // coordinator.schedule(new F(forward< Args >(args)...));
    coordinator.schedule(make_lambda_blocking_resumable(
        xi::bind(maker, forward< Args >(args)...)));
  }
}
}

struct acceptor2 : public core::generic_resumable, public resumable_fd {
  acceptor2()
      : xi::resumable_fd(::socket(AF_INET,
                                  SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                                  IPPROTO_TCP),
                         this) {
    struct sockaddr_in addr;
    ::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(19999);
    addr.sin_addr.s_addr = INADDR_ANY;
    int reuse            = 1;
    setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (bind(fd(), (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      perror("bind");
      exit(EXIT_FAILURE);
    }
    if (listen(fd(), 5) < 0) {
      perror("listen");
      exit(EXIT_FAILURE);
    }
  }

  void call() override {
    sockaddr_in* addr;
    socklen_t len = sizeof(addr);
    for (;;) {
      await_readable();
      printf("Readable!\n");
      for (;;) {
        int sock = ::accept4(
            fd(), (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (sock < 0) {
          printf("Sock: %d\n", sock);
          printf("Errno: %d\n", errno);
          if (errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
          }
          perror("accept");
          return;
        }
        printf("Connection! %d\n", sock);
        xi::core::spawn< channel2 >(sock);
      }
    }
  }
};

struct foo {
public:
  foo() {
    // printf("Created foo %p\n", this);
  }
  ~foo() {
    // printf("Destroyed foo %p\n", this);
  }
};

class ping_resumable : public core::generic_resumable {
  xi::core::channel< unique_ptr< foo > > _ch;

public:
  ping_resumable(xi::core::channel< unique_ptr< foo > > ch) : _ch(move(ch)) {
  }

protected:
  void call() {
    for (;;) {
      _ch.send(make_unique< foo >());
      printf("Ping got %p.\n", _ch.recv().get());
      sleep_for(1s);
    }
  }
};

class pong_resumable : public core::generic_resumable {
  xi::core::channel< unique_ptr< foo > > _ch;

public:
  pong_resumable(xi::core::channel< unique_ptr< foo > > ch) : _ch(move(ch)) {
  }

protected:
  void call() {
    for (auto&& i : _ch) {
      printf("Pong got %p.\n", i.get());
      _ch.send(make_unique< foo >());
      // sleep_for(1s);
    }
  }
};

class hog : public core::generic_resumable {
protected:
  void call() {
    for (;;) {
    }
  }
};

class simple_resumable : public core::generic_resumable {
protected:
  void call() {
    auto start = xi::high_resolution_clock::now();
    for (;;) {
      sleep_for(1s);
      auto end = xi::high_resolution_clock::now();
      printf("Hello, world! I slept for %luns.\n", (end - start).count());
      start = end;
    }
  }
};

// struct delayed_destruct {
//   ~delayed_destruct() {
//     while (true) {
//       printf("Still here!\n");
//       std::this_thread::sleep_for(10s);
//     }
//   }
// } DD;

int
main() {
  xi::core::runtime.set_max_cores(4);
  // auto ch = xi::core::make_channel< unique_ptr<foo>, 0 >();
  // for (auto i : range::to(1000)) {
  //   xi::core::spawn< ping_resumable >(ch);
  // }
  // for (auto i : range::to(10)) {
  //   xi::core::spawn< pong_resumable >(ch);
  // }
  xi::core::spawn< simple_resumable >();
  // xi::core::spawn< hog >();
  xi::core::spawn< acceptor2 >();
  // WORKER->push(new acceptor);
  // WORKER->run();
  return 0;
}
