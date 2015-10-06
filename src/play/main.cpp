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
#include "xi/io/buf/buffer_queue.h"

using ::boost::intrusive_ptr;
using namespace xi;
using namespace xi::async;
using namespace xi::async::libevent;
using namespace xi::io;

class fixed_stream_reader
    : public pipes::filter< io::socket_readable, error_code,
                            mut< buffer_queue >,
                            pipes::write_only< buffer::view > > {
  size_t _read_amount;
  mut< client_channel2< kInet, kTCP > > _channel;
  own< buffer_allocator > _alloc;
  buffer_queue _queue;

public:
  fixed_stream_reader(size_t read_amount,
                      mut< client_channel2< kInet, kTCP > > channel,
                      own< buffer_allocator > alloc)
      : _read_amount(read_amount), _channel(channel), _alloc(move(alloc)) {}

  void read(mut< context > cx, io::socket_readable) override {
    // std::cout << "Thread " << pthread_self() << " uses allocator "
    //           << address_of(_alloc) << std::endl;
    error_code error;
    while (true) {
      auto buf = _alloc->allocate(_read_amount);
      auto read =
          _channel->read(byte_range{buf->writable_bytes(), buf->length()});
      if (XI_UNLIKELY(read.has_error())) {
        error = read.error();
        break;
      }
      _queue.push_back(move(buf));
      if ((size_t)read < _read_amount) {
        // a good indication that we've exhausted the buffer
        break;
      }
    }
    // cx->forward_read(move(_buffer->make_view(_offset, read)));
    cx->forward_read(edit(_queue));
    if (XI_UNLIKELY((bool)error)) { process_error(cx, error); }
  }

  void process_error(mut< context > cx, error_code error) {
    static const error_code EAgain =
        make_error_code(std::errc::resource_unavailable_try_again);
    static const error_code EWould_block =
        make_error_code(std::errc::operation_would_block);

    if (error != EAgain && error != EWould_block) {
      // do nothing
      return;
    } else if (error == io::error::kEOF) {
      std::cout << "Channel closed by remote peer" << std::endl;
    } else { std::cout << "Channel error: " << error.message() << std::endl; }
    cx->forward_read(error);
    _channel->close();
  }

  void write(mut< context > cx, buffer::view rg) override {
    _channel->write(byte_range{(uint8_t *)rg.readable_bytes(), rg.length()});
  }
};

struct message {
  uint8_t version;
  uint8_t type;
  uint32_t size;
  buffer::view payload;
};

class message_decoder
    : public pipes::filter< pipes::read_only< mut< buffer_queue > >,
                            buffer::view, pipes::write_only< message > > {
public:
  void read(mut< context > cx, mut< buffer_queue > bq) override {
    buffer_queue::cursor c(bq);
    c.skip(sizeof(uint8_t) * 2);
    // auto size = c.read<uint32_t>();
    // cx->forward_write(move(rg));
  }
  void write(mut<context> cx, message msg) override {
  }
};

class range_echo : public pipes::filter< buffer::view > {
public:
  void read(mut< context > cx, buffer::view rg) override {
    cx->forward_write(move(rg));
  }
};

static thread_local own< arena_allocator > ALLOC =
    make< arena_allocator >(1 << 20);

using reactive_service = xi::async::sharded_service<
    xi::async::reactor_service< libevent::reactor > >;

class acceptor_handler
    : public pipes::filter<
          pipes::read_only< own< client_channel2< kInet, kTCP > > > > {
  mut< reactive_service > _reactive_service;
  own< core::executor_pool > _pool;

public:
  acceptor_handler(mut< reactive_service > rs, own< core::executor_pool > pool)
      : _reactive_service(rs), _pool(move(pool)) {}

  void read(mut< context > cx, own< client_channel2< kInet, kTCP > > ch) {
    _pool->post([&, ch = move(ch) ]() mutable {
      // ch->pipe()->push_back(make< stream_rpc_data_read_filter >(edit(ch)));
      ch->pipe()->push_back(
          make< fixed_stream_reader >(1 << 12, edit(ch), share(ALLOC)));
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

int main(int argc, char *argv[]) {
  auto k = make< core::launchable_kernel< core::thread_launcher > >();
  k->start(4, 1 << 20);
  auto pool = make_executor_pool(edit(k), {0, 1, 2, 3});
  // pool->post_on_all(fork_bomb(edit(pool)));
  // pool->post_on_all(fork_bomb_task(edit(pool)));
  // pool->post_on_all(fork_bloat< 2024 >(edit(pool)));
  // pool->post_on_all([p = share(pool), k=edit(k)] {
  //   std::cout << "Hello, world!" << std::endl;
  //   p->post_on_all([p = share(p)] { std::cout << "Hello, world!" <<
  //   std::endl; });
  //   // k->initiate_shutdown();
  // });

  auto r_service = make< reactive_service >(share(pool));
  r_service->start().then([&pool, &r_service] {
    auto acc = make< acceptor< kInet, kTCP > >();

    acc->bind(19999);
    acc->pipe()->push_back(
        make< acceptor_handler >(edit(r_service), share(pool)));

    pool->post([&r_service, acc = move(acc) ] {
      auto r = r_service->local()->reactor();
      r->attach_handler(move(acc));
      std::cout << "Acceptor attached." << std::endl;
    });
  });

  k->await_shutdown();
}
