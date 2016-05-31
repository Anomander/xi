#include "xi/ext/configure.h"
#include "xi/core/all.h"
#include "xi/io/error.h"
#include "xi/io/net/channel_options.h"
#include "xi/io/net/listener.h"
#include "xi/io/net/socket.h"
#include "xi/util/result.h"

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

using namespace xi;
using namespace xi::io;
using namespace xi::io;

struct cheat : public net::socket, public core::generic_resumable {
  cheat(int sock) : net::socket(sock) {
  }

  void call() override {
    char RESPONSE[] =
        "HTTP/1.1 200 OK\r\nContent-Length: 14\r\n\r\nHello World\r\n\r\n";
    for (;;) {
      auto r = write_exactly(RESPONSE, sizeof(RESPONSE));
      if (r <= 0) {
        // printf("Socket error: %s\n", r.to_error().message().c_str());
        break;
      }
    }
  }
};

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>

using namespace boost::accumulators;

thread_local struct {
  // accumulator_set< u64, stats< tag::mean, tag::tail_quantile< right > > >
  //     in_read{right_tail_cache_size = 10000};
  // accumulator_set< u64, stats< tag::mean, tag::tail_quantile< right > > >
  //     in_write{right_tail_cache_size = 10000};
  // accumulator_set< u64, stats< tag::mean, tag::tail_quantile< right > > >
  //     read_write{right_tail_cache_size = 10000};
  u64 in_read;
  u64 in_write;
  u64 read_write;
  u64 count;
} CHANNEL2_STAT;

struct channel2 : public net::socket, public core::v2::generic_resumable {
  channel2(int sock) : net::socket(sock) {
    set_option(io::net::option::tcp::no_delay::yes);
  }

  void call() override {
    vector< char > data(4 << 12);
    auto& stats = CHANNEL2_STAT;
    for (;;) {
      auto r_start = high_resolution_clock::now();
      auto r       = read_some(data.data(), data.size());
      auto r_end   = high_resolution_clock::now();
      stats.in_read += ((r_end - r_start).count());
      if (r > 0) {
        write_exactly(data.data(), r);
        auto w_end = high_resolution_clock::now();
        stats.in_write += ((w_end - r_end).count());
        stats.read_write += ((w_end - r_start).count());
        ++stats.count;
      } else {
        break;
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
      sleep(1s);
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
      sleep(1s);
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
      sleep(1s);
      auto end = xi::high_resolution_clock::now();
      printf("Hello, world! I slept for %luns.\n", (end - start).count());
      printf("%p -> %p\n", pthread_self(), &core::WORKER_STATS);
      printf("My worker has %lu tasks ready.\n",
             core::WORKER_STATS.tasks_ready);
      printf("My worker had %lu max tasks ready.\n",
             core::WORKER_STATS.max_tasks_ready);
      core::WORKER_STATS.max_tasks_ready = 0;
      start                              = end;
    }
  }
};

thread_local struct {
  nanoseconds delay;
  u64 count;
} TASK_STAT;

class simple_task : public core::v2::generic_resumable {
  void call() {
    for (;;) {
      auto start = xi::high_resolution_clock::now();
      core::v2::sleep(1s);
      auto& stat = TASK_STAT;
      stat.delay += xi::high_resolution_clock::now() - start;
      ++stat.count;
    }
  }
};

class simple_resumable_v2 : public core::v2::generic_resumable {
protected:
  void call() {
    for (;;) {
      auto start = xi::high_resolution_clock::now();
      core::v2::sleep(1s);
      auto end = xi::high_resolution_clock::now();
      // printf("Hello, world! I slept for %luns.\n", (end - start).count());
      {
        auto& stat = TASK_STAT;
        if (0 < stat.count) {
          printf("%p task\ncount: %lu\navg: %luns\n",
                 pthread_self(),
                 stat.count,
                 (stat.delay.count() / stat.count));
          stat.delay = 0ns;
          stat.count = 0;
        }
      }
      // {
      //   auto& stat = core::v2::RESUMABLE_STAT;
      //   if (0 < stat.running_count) {
      //     printf("%p resumable\nrunning: %luns\n",
      //            pthread_self(),
      //            (stat.running.count() / stat.running_count));
      //   }
      //   if (0 < stat.polling_count) {
      //     printf("%p resumable\npolling: %luns\n",
      //            pthread_self(),
      //            (stat.polling.count() / stat.polling_count));
      //     stat.polling_count = 0;
      //     stat.polling       = 0ns;
      //   }
      // }
      // {
      //   auto& stat = CHANNEL2_STAT;
      //   if (0 < stat.count) {
      //     printf("%p\nin read: %luns\nin write: %luns\nread write: %luns\n",
      //            pthread_self(),
      //            (stat.in_read / stat.count),
      //            (stat.in_write / stat.count),
      //            (stat.read_write / stat.count));
      //     // quantile(CHANNEL2_STAT.in_read, quantile_probability = 0.999));
      //   }
      // }
    }
  }
};

int
main() {
  xi::core::v2::runtime.set_cores(3);
  // auto ch = xi::core::make_channel< unique_ptr<foo>, 0 >();
  // for (auto i : range::to(1000)) {
  //   spawn< ping_resumable >(ch);
  // }
  // for (auto i : range::to(10)) {
  //   spawn< pong_resumable >(ch);
  // }
  xi::core::v2::spawn< simple_resumable_v2 >();
  xi::core::v2::spawn< simple_resumable_v2 >();
  xi::core::v2::spawn< simple_resumable_v2 >();
  xi::core::v2::spawn< simple_resumable_v2 >();
  for ([[gnu::unused]] auto i : range::to(300000)) {
    xi::core::v2::spawn< simple_task >();
  }
  // spawn< hog >();
  // spawn< acceptor >();
  xi::core::v2::spawn([] {
    printf("Accepting\n");
    net::listener l(19999);
    for (;;) {
      l.accept().then([](auto sock) {
        // for (auto&& sock : l.as_channel()) {
        printf("Connected\n");
        core::v2::spawn< channel2 >(sock);
        // }
      });
    }
  });
  return 0;
}
