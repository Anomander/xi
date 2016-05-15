#include "xi/core/kernel_utils.h"
#include "xi/core/memory/memory.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/net/async_channel.h"
#include "xi/io/proto/all.h"

#include "xi/server_bootstrap.h"

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

struct http2_pipe_factory : public net::pipe_factory< net::kInet, net::kTCP > {
  own< pipe_t > create_pipe(net::stream_client_socket s) override {
    return make< http2_pipe >(move(s));
  }
};

int
main(int argc, char* argv[]) {
  server_bootstrap::create()
      .configure< core::boost_po_backend >(argc, argv)
      .bind< net::tcp_acceptor_pipe >((argc > 2 ? atoi(argv[1]) : 19999),
                                      [](mut< net::tcp_acceptor_pipe > pipe) {
                                        pipe->set_pipe_factory(
                                            make< http2_pipe_factory >());
                                      })
      .bind< net::udp_datagram_pipe >((argc > 2 ? atoi(argv[1]) : 19999),
                                      [](mut< net::udp_datagram_pipe > pipe) {
                                        pipe->push_back(make< range_echo >());
                                      })
      .before_shutdown([] {
        core::post_on_all([&] {
          static spin_lock sl;
          auto lock = make_unique_lock(sl);
          std::cout << pthread_self() << "\nconns : " << stats.connections
                    << "\nreads : " << stats.reads
                    << "\nr_bytes : " << stats.r_bytes
                    << "\nwrites : " << stats.writes
                    << "\nw_bytes : " << stats.w_bytes << std::endl;
          auto mstats = xi::core::memory::stats();
          std::cout << "MALLOC:\n"
                    << "\nallocs : " << mstats.mallocs
                    << "\nfrees : " << mstats.frees
                    << "\ncross_cpu_frees : " << mstats.cross_cpu_frees
                    << "\ntotal_memory : " << mstats.total_memory
                    << "\nfree_memory : " << mstats.free_memory << std::endl;
        });
      })
      .run([argc, argv] {
        // std::cout << "I'm running!" << std::endl;
        // auto conn = make< net::tcp_connector_pipe >();
        // auto ep = net::ip4_endpoint(argc > 2 ? atoi(argv[1]) : 19999);
        // std::cout << "ep: " << ep.to_string() << std::endl;
        // conn->connect(move(ep)).then([](auto) {
        //   std::cout << "Connected!" << std::endl;
        // });
      });
}
