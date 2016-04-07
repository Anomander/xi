#include "xi/core/epoll/reactor.h"
#include "xi/core/kernel_utils.h"
#include "xi/core/launchable_kernel.h"
#include "xi/core/thread_launcher.h"
#include "xi/hw/hardware.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/net/async_channel.h"
#include "xi/io/proto/all.h"
#include "xi/core/memory/memory.h"

#include "xi/core/memory/memory.h"

#include <signal.h>

using namespace xi;
using namespace xi::io;

alignas(64) static thread_local struct {
  usize connections = 0;
  usize reads       = 0;
  usize r_bytes     = 0;
  usize writes      = 0;
  usize w_bytes     = 0;
} stats;

class logging_filter : public pipes::filter< own< buffer >,
                                             net::ip_datagram,
                                             net::unix_datagram > {
public:
  logging_filter() {
    ++stats.connections;
  }
  void read(mut< context > cx, own< buffer > b) override {
    ++stats.reads;
    stats.r_bytes += b->size();
    cx->forward_read(move(b));
  }

  void write(mut< context > cx, own< buffer > b) override {
    ++stats.writes;
    stats.w_bytes += b->size();
    cx->forward_write(move(b));
  }
  void read(mut< context > cx, net::ip_datagram b) override {
    ++stats.reads;
    stats.r_bytes += b.data->size();
    cx->forward_read(move(b));
  }
  void read(mut< context > cx, net::unix_datagram b) override {
    ++stats.reads;
    stats.r_bytes += b.data->size();
    cx->forward_read(move(b));
  }
  void write(mut< context > cx, net::ip_datagram b) override {
    ++stats.writes;
    stats.w_bytes += b.data->size();
    cx->forward_write(move(b));
  }
  void write(mut< context > cx, net::unix_datagram b) override {
    ++stats.writes;
    stats.w_bytes += b.data->size();
    cx->forward_write(move(b));
  }
};

class range_echo : public pipes::filter< own< buffer >,
                                         net::ip_datagram,
                                         net::unix_datagram > {
public:
  void read(mut< context > cx, own< buffer > b) override {
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
  http2_pipe(A&&... args) : net::tcp_stream_pipe(forward< A >(args)...) {
    this->push_back(make< logging_filter >());
    this->push_back(make< range_echo >());
    // push_back(make< proto::http2::codec >(alloc()));
  }
};

template < net::address_family af, net::protocol p >
class pipe_initialize_filter
    : public pipes::filter<
          pipes::in< own< net::client_pipe< net::kInet, net::kTCP > > > > {
  mut< core::executor_pool > _pool;

protected:
  using pipe = net::client_pipe< af, p >;
  using base = pipe_initialize_filter;

  void read(mut< context > cx, own< pipe > ch) final override {
    _pool->post([ this, ch = move(ch) ]() mutable {
      initialize_pipe(edit(ch));
      shard()->reactor()->attach_handler(move(ch));
    });
  }

public:
  pipe_initialize_filter(mut< core::executor_pool > pool) : _pool(pool) {
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

auto k = make<
    core::launchable_kernel< core::thread_launcher, core::epoll::reactor > >();

int
main(int argc, char* argv[]) {
  // malloc(16 << 20);
  // malloc(16 << 20);
  // malloc(16 << 20);
  // return 0;
  signal(SIGINT, [](int sig) { k->initiate_shutdown(); });
  signal(SIGPIPE,
         [](int sig) { std::cout << "Ignoring SIGPIPE." << std::endl; });

  own< core::executor_pool > pool;
  spin_lock sl;
  k->start(argc > 2 ? atoi(argv[2]) : 1, 1 << 20).then([&, argc, argv] {
    pool = make_executor_pool(edit(k));
    // void* p[100];
    // usize total_size = 0;
    // for (auto i : range::to(100)) {
    //   xi::core::this_shard->post([i, &p, &total_size] {
    //     p[i] = malloc(16 << 20);
    //     total_size += malloc_usable_size(p[i]);
    //     std::cout << "malloc(" << p[i] << ") sz: " << total_size << std::endl;
    //   });
    // }
    // for (auto i : range::to(100)) {
    //   xi::core::this_shard->post([i, &p] {
    //     std::cout << "free(" << p[i] << ")" << std::endl;
    //     free(p[i]);
    //   });
    // }
    // xi::core::this_shard->post([] { std::cout << "done" << std::endl; });
    // return;

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
    acc->push_back(make< http2_handler >(edit(pool)));
    // acc->set_child_options(net::option::tcp::no_delay::yes);
    // acc->set_child_options(net::option::socket::receive_buffer::val(1 << 20));
    // acc->set_child_options(net::option::socket::send_buffer::val(1 << 16));

    pool->post([ acc = move(acc), dgram = move(dgram) ] {
      shard()->reactor()->attach_handler(move(acc));
      shard()->reactor()->attach_handler(move(dgram));
      std::cout << "Acceptor attached." << std::endl;
    });
    k->before_shutdown().then([&] {
      pool->post_on_all([&] {
        auto lock = make_unique_lock(sl);
        std::cout << pthread_self() << "\nconns : " << stats.connections
                  << "\nreads : " << stats.reads
                  << "\nr_bytes : " << stats.r_bytes
                  << "\nwrites : " << stats.writes
                  << "\nw_bytes : " << stats.w_bytes << std::endl;
        auto mstats = xi::core::memory::stats();
        std::cout << "MALLOC:\n" << "\nallocs : " << mstats.mallocs
                  << "\nfrees : " << mstats.frees
                  << "\ncross_cpu_frees : " << mstats.cross_cpu_frees
                  << "\ntotal_memory : " << mstats.total_memory
                  << "\nfree_memory : " << mstats.free_memory << std::endl;
      });
    });
  });

  struct statistics {
    u64 mallocs;
    u64 frees;
    u64 cross_cpu_frees;
    usize total_memory;
    usize free_memory;
    u64 reclaims;
  };
  k->await_shutdown();
}
