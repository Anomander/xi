#include "xi/io/async_channel.h"
#include "xi/async/libevent/reactor.h"
#include "xi/hw/hardware.h"
#include "xi/core/launchable_kernel.h"
#include "xi/core/thread_launcher.h"
#include "xi/core/kernel_utils.h"
#include "xi/async/sharded_service.h"
#include "xi/async/reactor_service.h"
#include "xi/io/buf/heap_buf.h"
#include "xi/io/buf/arena_allocator.h"
#include "xi/io/buf/chain.h"
#include "xi/io/buf/view.h"
#include "xi/io/buf/cursor.h"
#include "xi/io/buf/buf.h"
#include "xi/io/byte_buffer.h"
#include "xi/io/byte_buffer_chain.h"
#include "xi/io/byte_buffer_allocator.h"

#include <signal.h>

using namespace xi;
using namespace xi::async;
using namespace xi::async::libevent;
using namespace xi::io;

alignas(64) static thread_local struct {
  usize connections = 0;
  usize reads = 0;
  usize r_bytes = 0;
  usize writes = 0;
  usize w_bytes = 0;
} stats;

class channel_handshake_filter : public pipes::filter<> {};

class logging_filter : public pipes::filter< own< byte_buffer > > {
public:
  logging_filter() { ++stats.connections; }
  void read(mut< context > cx, own< byte_buffer > b) override {
    ++stats.reads;
    stats.r_bytes += b->size();
    cx->forward_read(move(b));
  }

  void write(mut< context > cx, own< byte_buffer > b) override {
    ++stats.writes;
    stats.w_bytes += b->size();
    cx->forward_write(move(b));
  }
};

class range_echo : public pipes::filter< own< byte_buffer > > {
public:
  void read(mut< context > cx, own< byte_buffer > b) override {
    // std::cout << "Got buffer " << b->size() << std::endl;
    cx->forward_write(move(b));
  }
};

namespace http2 {
struct[[gnu::packed]] preamble {
  u32 length : 24;
  u8 type : 8;
  u8 flags : 8;
  bool unset : 1;
  u32 stream : 31;
};
static_assert(sizeof(preamble) == 9, "");
}

class frame_decoder : public pipes::filter< pipes::in< own< byte_buffer > > > {
  byte_buffer::chain _chain;

  void read(mut< context > cx, own< byte_buffer > b) final override {
    _chain.push_back(move(b));
    while (auto len = decode(edit(_chain))) {
      if (len && _chain.size() >= len) {
        byte_buffer::chain c;
        c.push_back(_chain.split(len));
        cx->forward_read(move(c.pop_front()));
      } else { break; }
    }
  }

public:
  virtual ~frame_decoder() = default;
  virtual usize decode(mut< byte_buffer::chain > in) = 0;
};

class fixed_length_frame_decoder : public frame_decoder {
  usize _length = 4 << 12;

public:
  fixed_length_frame_decoder() = default;
  fixed_length_frame_decoder(usize l) : _length(l){};

  usize decode(mut< byte_buffer::chain >) override { return _length; }
};

class length_field_frame_decoder : public frame_decoder {
public:
  length_field_frame_decoder() = default;

  usize decode(mut< byte_buffer::chain > in) override {
    if (in->size() < sizeof(http2::preamble)) { return 0; }
    http2::preamble hdr;
    in->read(byte_range{hdr});
    in->trim_start(sizeof(hdr));
    std::cout << "length:" << hdr.length << std::endl;
    return hdr.length;
  }
};

using reactive_service = xi::async::sharded_service<
    xi::async::reactor_service< libevent::reactor > >;

class acceptor_handler
    : public pipes::filter<
          pipes::in< own< client_channel2< kInet, kTCP > > > > {
  mut< reactive_service > _reactive_service;

public:
  acceptor_handler(mut< reactive_service > rs) : _reactive_service(rs) {}

  void read(mut< context > cx, own< client_channel2< kInet, kTCP > > ch) {
    _reactive_service->post([ch = move(ch) ]() mutable {
      ch->pipe()->push_back(make< logging_filter >());
      ch->pipe()->push_back(make< fixed_length_frame_decoder >(1 << 12));
      // ch->pipe()->push_back(make< length_field_frame_decoder >());
      ch->pipe()->push_back(make< range_echo >());
      auto reactor = _reactive_service->local()->reactor();
      reactor->attach_handler(move(ch));
    });
  }

  void read(mut< context > cx, exception_ptr ex) {
    try {
      rethrow_exception(ex);
    } catch (std::exception &e) {
      std::cout << "Exception: " << e.what() << std::endl;
    } catch (...) { std::cout << "Undefined exception" << std::endl; }
  }
};

auto k = make< core::launchable_kernel< core::thread_launcher > >();

int main(int argc, char *argv[]) {

  struct sigaction SIGINT_action;
  SIGINT_action.sa_handler = [](int sig) { k->initiate_shutdown(); };
  sigemptyset(&SIGINT_action.sa_mask);
  SIGINT_action.sa_flags = 0;
  sigaction(SIGINT, &SIGINT_action, nullptr);

  sigblock(sigmask(SIGPIPE));
  k->start(6, 1 << 20);
  auto pool = make_executor_pool(edit(k));

  auto r_service = make< reactive_service >(share(pool));
  r_service->start().then([&pool, &r_service] {
    auto acc = make< acceptor< kInet, kTCP > >();

    std::cout << "Acceptor created." << std::endl;
    try {
      acc->bind(19999);
    } catch (ref< std::exception > e) {
      std::cout << "Bind error: " << e.what() << std::endl;
      exit(1);
    }
    std::cout << "Acceptor bound." << std::endl;
    acc->pipe()->push_back(make< acceptor_handler >(edit(r_service)));

    pool->post([&r_service, acc = move(acc) ] {
      auto r = r_service->local()->reactor();
      r->attach_handler(move(acc));
      std::cout << "Acceptor attached." << std::endl;
    });
  });
  spin_lock sl;
  k->before_shutdown().then([&] {
    pool->post_on_all([&] {
      auto lock = make_unique_lock(sl);
      std::cout << pthread_self() << "\nconns : " << stats.connections
                << "\nreads : " << stats.reads
                << "\nr_bytes : " << stats.r_bytes
                << "\nwrites : " << stats.writes
                << "\nw_bytes : " << stats.w_bytes << std::endl;
    });
  });

  k->await_shutdown();
}
