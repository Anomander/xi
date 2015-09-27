#define XI_USER_UPSTREAM_MESSAGE_ROOT boost::intrusive_ptr< protocol_message >

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include "xi/io/async_channel.h"
#include "xi/io/pipeline/util.h"
#include "xi/async/libevent/reactor.h"
#include "xi/hw/hardware.h"
#include "xi/core/launchable_kernel.h"
#include "xi/core/thread_launcher.h"
#include "xi/core/kernel_utils.h"
#include "xi/async/reactor_service.h"

#include <malloc.h>

using ::boost::intrusive_ptr;
using namespace xi;
using namespace xi::async;
using namespace xi::async::libevent;
using namespace xi::io;

class data_handler : public pipeline::pipeline_handler {
public:
  void handle_event(mut< pipeline::handler_context > cx,
                    pipeline::data_available msg) override {
    // std::cout << "Message received." << std::endl;
    expected< int > read = 0;
    while (true) {
      if (nullptr != _message_cursor) {
        // std::cout << "Cursor:" << (void*)_message_cursor << " size:" <<
        // _remaining_size << std::endl;
        read = cx->get_channel()->read(
            {byte_range{_message_cursor, _remaining_size},
             byte_range{_header_bytes, sizeof(protocol_header)}});
      } else { read = cx->get_channel()->read({header_byte_range()}); }

      if (cx->get_channel()->is_closed()) { break; }
      auto bytes_read = static_cast< size_t >(read);
      // std::cout << "I just read " << bytes_read << " bytes." << std::endl;
      if (_message_cursor) {
        if (bytes_read >= _remaining_size) {
          // std::cout << "Complete message" << std::endl;
          auto *pm = _current_message;
          _message_cursor = nullptr;
          _current_message = nullptr;
          // auto protocol_message = new (message - sizeof(protocol_header))
          // protocol_message;
          // cx->forward(pipeline::message_read{make< data_message
          // >(protocol_message)});
          cx->forward(pipeline::user_upstream_event{
              intrusive_ptr< protocol_message >(pm)});
          bytes_read -= _remaining_size;
          _remaining_size = 0;
          _header_cursor = _header_bytes;
        } else {
          _message_cursor += bytes_read;
          _remaining_size -= bytes_read;
          bytes_read = 0;
        }
        // std::cout << _remaining_size << " bytes left to read." << std::endl;
      }
      // std::cout << bytes_read << " unconsumed bytes left." << std::endl;
      if (bytes_read > 0) {
        // for (size_t i = 0; i < bytes_read; ++i) {
        //   std::cout << (int)*(_header_cursor + i) << " ";
        // }
        // std::cout << std::endl;
        // std::cout << "Header cursor: " << (void*)_header_cursor << std::endl;
        // std::cout << "Header bytes: " << (void*)_header_bytes << std::endl;
        _header_cursor += bytes_read;
        if (_header_cursor == end(_header_bytes)) {
          // std::cout << "New header" << std::endl;
          auto *hdr = reinterpret_cast< protocol_header * >(_header_bytes);

          // std::cout << "size: " << hdr->size << std::endl;
          // std::cout << "version: " << hdr->version << std::endl;
          // std::cout << "type: " << hdr->type << std::endl;

          _remaining_size = hdr->size;
          auto message = (protocol_message *)::malloc(_remaining_size +
                                                      sizeof(protocol_message));
          ::memcpy(&(message->_header), hdr, sizeof(protocol_header));
          _current_message = message;
          _message_cursor = message->_data;
          _header_cursor = begin(_header_bytes);
        }
      }
    }
  }

private:
  byte_range header_byte_range() const noexcept {
    return byte_range{_header_cursor,
                      sizeof(protocol_header) -
                          (_header_cursor - begin(_header_bytes))};
  }

private:
  uint8_t _header_bytes[sizeof(protocol_header)];
  protocol_message *_current_message = nullptr;
  uint8_t *_message_cursor = nullptr;
  uint8_t *_header_cursor = _header_bytes;
  size_t _remaining_size = 0UL;
};

class user_message_handler : public pipeline::pipeline_handler {
protected:
#ifdef XI_USER_UPSTREAM_MESSAGE_ROOT
  void handle_event(mut< pipeline::handler_context > cx,
                    pipeline::user_upstream_event event) final override {
    upstream_message(cx, event.root);
  }
#endif // XI_USER_UPSTREAM_MESSAGE_ROOT
#ifdef XI_USER_DOWNSTREAM_MESSAGE_ROOT
  void handle_event(mut< pipeline::handler_context > cx,
                    pipeline::user_downstream_event event) final override {
    downstream_message(cx, event.root);
  }
#endif // XI_USER_DOWNSTREAM_MESSAGE_ROOT
public:
#ifdef XI_USER_UPSTREAM_MESSAGE_ROOT
  virtual void upstream_message(mut< pipeline::handler_context > cx,
                                XI_USER_UPSTREAM_MESSAGE_ROOT msg) = 0;
#endif // XI_USER_UPSTREAM_MESSAGE_ROOT
#ifdef XI_USER_DOWNSTREAM_MESSAGE_ROOT
  virtual void downstream_message(mut< pipeline::handler_context > cx,
                                  XI_USER_DOWNSTREAM_MESSAGE_ROOT msg) = 0;
#endif // XI_USER_DOWNSTREAM_MESSAGE_ROOT
};

class my_handler : public user_message_handler {
public:
  void upstream_message(mut< pipeline::handler_context > cx,
                        intrusive_ptr< protocol_message > msg) override {
    // std::cout << "User event!" << std::endl;
    cx->get_channel()->write(
        byte_range{(uint8_t *)&msg->_header,
                   msg->header().size + sizeof(protocol_header)});
  }
};

class message_handler
    : public pipeline::simple_inbound_pipeline_handler< data_message > {
public:
  void message_received(mut< pipeline::handler_context > cx,
                        own< data_message > msg) override {
    // std::cout << "Message received." << std::endl;
    cx->get_channel()->write(move(msg));
  }
  void handle_event(mut< pipeline::handler_context > cx,
                    pipeline::channel_error error) override {
    std::cout << "Channel error: " << error.error().message() << std::endl;
    cx->get_channel()->close();
  }
};

thread_local size_t count;

struct fork_bomb_task : public xi::core::task {
  fork_bomb_task(mut< xi::core::executor_pool > p) : _pool(p) {}
  void run() override {
    count++;
    if (count % 10000000 == 0) {
      std::cout << pthread_self() << " : " << count << std::endl;
    }
    _pool->post_on_all(fork_bomb_task(_pool));
  };

  mut< xi::core::executor_pool > _pool;
};

struct fork_bomb {
  fork_bomb(mut< xi::core::executor_pool > p) : _pool(p) {}
  void operator()() {
    count++;
    if (count % 10000000 == 0) {
      std::cout << pthread_self() << " : " << count << std::endl;
    }
    _pool->post(fork_bomb(_pool));
    // _pool->post_on_all(fork_bomb(_pool));
  };

  mut< xi::core::executor_pool > _pool;
};

template < size_t N > struct fork_bloat {
  fork_bloat(mut< xi::core::executor_pool > p) : _pool(p) {}
  void operator()() {
    std::cout << "Fork_bloat running " << std::endl;
    count++;
    if (count % 10 == 0) {
      std::cout << pthread_self() << " : " << count << std::endl;
    }
    _pool->post_on_all(fork_bloat< N >(_pool));
  };

  mut< xi::core::executor_pool > _pool;
  char bloat[N];
};

class rpc_stream_reader {
  mut< client_channel2< kInet, kTCP > > _channel;
  uint8_t _header_bytes[sizeof(protocol_header)];
  protocol_message *_current_message = nullptr;
  uint8_t *_message_cursor = nullptr;
  uint8_t *_header_cursor = _header_bytes;
  size_t _remaining_size = 0UL;

private:
  byte_range header_byte_range() const noexcept {
    return byte_range{_header_cursor,
                      sizeof(protocol_header) -
                          (_header_cursor - begin(_header_bytes))};
  }

public:
  rpc_stream_reader(mut< client_channel2< kInet, kTCP > > channel)
      : _channel(channel) {}

  template < class func_msg, class func_err >
  void read(func_msg &&on_message_callback, func_err &&on_error_callback) {
    expected< int > read = 0;
    while (true) {
      if (nullptr != _message_cursor) {
        read = _channel->read(
            {byte_range{_message_cursor, _remaining_size},
             byte_range{_header_bytes, sizeof(protocol_header)}});
      } else { read = _channel->read({header_byte_range()}); }

      if (read.has_error()) {
        static const error_code EAgain =
            make_error_code(std::errc::resource_unavailable_try_again);
        static const error_code EWould_block =
            make_error_code(std::errc::operation_would_block);

        auto error = read.error();
        if (error != EAgain && error != EWould_block) {
          on_error_callback(error);
        }
        return;
      }
      if (read == 0) { return; }
      auto bytes_read = static_cast< size_t >(read);
      intrusive_ptr< protocol_message > p_message = nullptr;
      if (_message_cursor) {
        if (bytes_read >= _remaining_size) {
          p_message = _current_message;
          _message_cursor = nullptr;
          _current_message = nullptr;
          bytes_read -= _remaining_size;
          _remaining_size = 0;
          _header_cursor = _header_bytes;
        } else {
          _message_cursor += bytes_read;
          _remaining_size -= bytes_read;
          bytes_read = 0;
        }
      }
      if (bytes_read > 0) {
        _header_cursor += bytes_read;
        if (_header_cursor == end(_header_bytes)) {
          auto *hdr = reinterpret_cast< protocol_header * >(_header_bytes);

          _remaining_size = hdr->size;
          auto message = (protocol_message *)::malloc(_remaining_size +
                                                      sizeof(protocol_message));
          ::memcpy(&(message->_header), hdr, sizeof(protocol_header));
          _current_message = message;
          _message_cursor = message->_data;
          _header_cursor = begin(_header_bytes);
        }
      }
      if (p_message) {
        on_message_callback(p_message);
        p_message = nullptr;
      }
    }
  }
};

class buf {
  uint8_t *_data;
  size_t _size;

public:
  buf(size_t sz) {
    _data = (uint8_t *)::malloc(sz);
    _size = ::malloc_usable_size(_data);
  }
};

class stream_rpc_data_read_filter
    : public pipe2::filter< intrusive_ptr< protocol_message >,
                            io::socket_readable, error_code > {
  rpc_stream_reader _reader;
  mut< client_channel2< kInet, kTCP > > _channel;

public:
  stream_rpc_data_read_filter(mut< client_channel2< kInet, kTCP > > channel)
      : _reader(channel), _channel(channel) {}

  // void write(mut<Context>cx, intrusive_ptr<Protocol_message> msg) override {
  //   _channel->write();
  // }
  void read(mut< context > cx, io::socket_readable) override {
    _reader.read([cx, this](auto msg) {
                   // cx->forward_read(move(msg));
                   _channel->write(byte_range{(uint8_t *)&msg->_header,
                                              msg->header().size +
                                                  sizeof(protocol_header)});
                 },
                 [cx, this](auto error) {
                   if (error == io::error::kEOF) {
                     std::cout << "Channel closed by remote peer" << std::endl;
                   } else {
                     std::cout << "Channel error: " << error.message()
                               << std::endl;
                     cx->forward_read(error);
                   }
                   _channel->close();
                 });
  }
};

using reactive_service =
    xi::async::service< xi::async::reactor_service< libevent::reactor > >;

class acceptor_handler
    : public pipe2::filter<
          pipe2::read_only< own< client_channel2< kInet, kTCP > > > > {
  mut< reactive_service > _reactive_service;

public:
  acceptor_handler(mut< reactive_service > rs) : _reactive_service(rs) {}

  void read(mut< context > cx, own< client_channel2< kInet, kTCP > > ch) {
    std::cout << "Channel ACCEPTED!" << std::endl;
    ch->pipe()->push_back(make< stream_rpc_data_read_filter >(edit(ch)));
    auto reactor = _reactive_service->local()->reactor();
    reactor->attach_handler(move(ch));
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
    auto ch = make< server_channel< kInet, kTCP > >();
    auto acc = make< acceptor< kInet, kTCP > >();

    // ch->bind(19999);
    acc->bind(19990);
    acc->pipe()->push_back(make< acceptor_handler >(edit(r_service)));

    ch->child_handler(pool->wrap([&r_service](auto ch) {
      auto r = r_service->local()->reactor();
      ch->get_pipeline()->push_back(make< data_handler >());
      ch->get_pipeline()->push_back(make< my_handler >());
      r->attach_handler(move(ch));
      std::cout << "Local reactor @ " << address_of(r) << std::endl;
    }));

    pool->post([&, ch = move(ch), acc = move(acc) ] {
      auto r = r_service->local()->reactor();
      r->attach_handler(move(ch));
      r->attach_handler(move(acc));
      std::cout << "Acceptor attached." << std::endl;
    });
  });

  k->await_shutdown();
}
