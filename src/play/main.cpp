#include "xi/async/libevent/reactor.h"
#include "xi/async/reactor_service.h"
#include "xi/async/sharded_service.h"
#include "xi/core/kernel_utils.h"
#include "xi/core/launchable_kernel.h"
#include "xi/core/thread_launcher.h"
#include "xi/hw/hardware.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/net/async_channel.h"
#include "xi/io/proto/all.h"

#include <signal.h>

#include <boost/timer/timer.hpp>

using namespace xi;
using namespace xi::async;
using namespace xi::async::libevent;
using namespace xi::io;

alignas(64) static thread_local struct {
  usize connections = 0;
  usize reads       = 0;
  usize r_bytes     = 0;
  usize writes      = 0;
  usize w_bytes     = 0;
} stats;

class logging_filter
    : public pipes::filter< buffer, net::ip_datagram, net::unix_datagram > {
public:
  logging_filter() {
    ++stats.connections;
  }
  void read(mut< context > cx, buffer b) override {
    ++stats.reads;
    stats.r_bytes += b.size();
    cx->forward_read(move(b));
  }

  void write(mut< context > cx, buffer b) override {
    ++stats.writes;
    stats.w_bytes += b.size();
    cx->forward_write(move(b));
  }
  void read(mut< context > cx, net::ip_datagram b) override {
    ++stats.reads;
    stats.r_bytes += b.data.size();
    cx->forward_read(move(b));
  }
  void read(mut< context > cx, net::unix_datagram b) override {
    ++stats.reads;
    stats.r_bytes += b.data.size();
    cx->forward_read(move(b));
  }
  void write(mut< context > cx, net::ip_datagram b) override {
    ++stats.writes;
    stats.w_bytes += b.data.size();
    cx->forward_write(move(b));
  }
  void write(mut< context > cx, net::unix_datagram b) override {
    ++stats.writes;
    stats.w_bytes += b.data.size();
    cx->forward_write(move(b));
  }
};

class range_echo
    : public pipes::filter< buffer, net::ip_datagram, net::unix_datagram > {
public:
  void read(mut< context > cx, buffer b) override {
    // std::cout << "Got buffer " << b->size() << std::endl;
    cx->forward_write(move(b));
  }
  void read(mut< context > cx, net::ip_datagram b) override {
    // std::cout << "Got datagram " << b.data.size() << std::endl;
    cx->forward_write(move(b));
  }
  void read(mut< context > cx, net::unix_datagram b) override {
    // std::cout << "Got unix datagram " << b.data.size() << std::endl;
    cx->forward_write(move(b));
  }
};

class http2_pipe : public net::tcp_stream_pipe {
public:
  template < class... A >
  http2_pipe(A&&... args) : client_pipe(forward< A >(args)...) {
    push_back(make< logging_filter >());
    push_back(make< range_echo >());
    // push_back(make< proto::http2::codec >(alloc()));
  }
};

using reactive_service = xi::async::sharded_service<
    xi::async::reactor_service< libevent::reactor > >;

template < net::address_family af, net::protocol p >
class pipe_initialize_filter
    : public pipes::filter<
          pipes::in< own< net::client_pipe< net::kInet, net::kTCP > > > > {
  mut< reactive_service > _reactive_service;
  mut< core::executor_pool > _pool;

protected:
  using pipe = net::client_pipe< af, p >;
  using base = pipe_initialize_filter;

  void read(mut< context > cx, own< pipe > ch) final override {
    _pool->post([ this, rs = _reactive_service, ch = move(ch) ]() mutable {
      initialize_pipe(edit(ch));
      rs->local()->reactor()->attach_handler(move(ch));
    });
  }

public:
  pipe_initialize_filter(mut< reactive_service > rs,
                         mut< core::executor_pool > pool)
      : _reactive_service(rs), _pool(pool) {
  }

  virtual ~pipe_initialize_filter()            = default;
  virtual void initialize_pipe(mut< pipe > ch) = 0;
};

struct http2_pipe_factory : public net::pipe_factory< net::kInet, net::kTCP > {
  own< pipe_t > create_pipe(net::stream_client_socket s) override {
    return make< http2_pipe >(move(s));
  }
};

class http2_handler : public pipe_initialize_filter< net::kInet, net::kTCP > {
public:
  using base::base;

  void initialize_pipe(mut< pipe > ch) override {
  }
};

auto k = make< core::launchable_kernel< core::thread_launcher > >();

struct foo {
  int i[1000];

  foo(int _) {
  }
};

int
main(int argc, char* argv[]) {
  signal(SIGINT, [](int sig) { k->initiate_shutdown(); });
  signal(SIGPIPE, [](auto) { std::cout << "Ignoring SIGPIPE." << std::endl; });

  own< core::executor_pool > pool;
  own< reactive_service > r_service;
  spin_lock sl;
  k->start(argc > 2 ? atoi(argv[2]) : 1, 1 << 20).then([&, argc, argv] {
    pool = make_executor_pool(edit(k));

    r_service = make< reactive_service >(share(pool));
    r_service->start().then([=, &pool, &r_service] {
      auto acc   = make< net::tcp_acceptor_pipe >();
      auto dgram = make< net::udp_datagram_pipe >();
      // auto dgram = make< net::unix_datagram_pipe >();
      dgram->push_back(make< range_echo >());

      std::cout << "Acceptor created." << std::endl;
      try {
        acc->bind(argc > 1 ? atoi(argv[1]) : 19999);
        // auto path = argc > 1 ? argv[1] : "/tmp/xi.sock";
        // unlink(path); // FIXME: will do for now
        // dgram->bind(path);
        dgram->bind(argc > 1 ? atoi(argv[1]) : 19999);
      } catch (ref< std::exception > e) {
        std::cout << "Bind error: " << e.what() << std::endl;
        exit(1);
      }
      std::cout << "Acceptor bound." << std::endl;
      acc->set_pipe_factory(make< http2_pipe_factory >());
      acc->push_back(make< http2_handler >(edit(r_service), edit(pool)));
      acc->set_child_options(net::option::tcp::no_delay::yes);

      pool->post([&r_service, acc = move(acc), dgram = move(dgram) ] {
        auto r = r_service->local()->reactor();
        r->attach_handler(move(acc));
        r->attach_handler(move(dgram));
        std::cout << "Acceptor attached." << std::endl;
      });
    });
    k->before_shutdown().then([&] {
      r_service->stop();
      pool->post_on_all([&] {
        auto lock = make_unique_lock(sl);
        std::cout << pthread_self() << "\nconns : " << stats.connections
                  << "\nreads : " << stats.reads
                  << "\nr_bytes : " << stats.r_bytes
                  << "\nwrites : " << stats.writes
                  << "\nw_bytes : " << stats.w_bytes << std::endl;
      });
    });
  });

  k->await_shutdown();
}
