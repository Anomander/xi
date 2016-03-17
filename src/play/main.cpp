#include "xi/io/net/async_channel.h"
#include "xi/async/libevent/reactor.h"
#include "xi/hw/hardware.h"
#include "xi/core/launchable_kernel.h"
#include "xi/core/thread_launcher.h"
#include "xi/core/kernel_utils.h"
#include "xi/async/sharded_service.h"
#include "xi/async/reactor_service.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"

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

class logging_filter : public pipes::filter< mut< buffer > > {
public:
  logging_filter() {
    ++stats.connections;
  }
  void read(mut< context > cx, mut< buffer > b) override {
    ++stats.reads;
    stats.r_bytes += b->size();
    cx->forward_read(move(b));
  }

  void write(mut< context > cx, mut< buffer > b) override {
    ++stats.writes;
    stats.w_bytes += b->size();
    cx->forward_write(move(b));
  }
};

class range_echo : public pipes::filter< mut< buffer > > {
public:
  void read(mut< context > cx, mut< buffer > b) override {
    // std::cout << "Got buffer " << b->size() << std::endl;
    cx->forward_write(b);
  }
};

namespace http2 {

static constexpr const char INITIATION_STRING[] =
    "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

enum class state : u8 {
  READ_CONNECTION_PREFACE,
  READ_FRAME_HEADER,
  READ_FRAME_PADDING,
  READ_DATA_FRAME = 0xF0,
  READ_HEADERS_FRAME = 0xF1,
  READ_PRIORITY_FRAME = 0xF2,
  READ_RST_STREAM_FRAME = 0xF3,
  READ_SETTINGS_FRAME = 0xF4,
  READ_PUSH_PROMISE_FRAME = 0xF5,
  READ_PING_FRAME = 0xF6,
  READ_GOAWAY_FRAME = 0xF7,
  READ_WINDOW_UPDATE_FRAME = 0xF8,
  READ_CONTINUATION_FRAME = 0xF9,
  CONNECTION_ERROR = 0xFF,
} _state = state::READ_CONNECTION_PREFACE;

enum class frame : u8 {
  DATA_FRAME = 0X00,
  HEADERS_FRAME = 0X01,
  PRIORITY_FRAME = 0X02,
  RST_STREAM_FRAME = 0X03,
  SETTINGS_FRAME = 0X04,
  PUSH_PROMISE_FRAME = 0X05,
  PING_FRAME = 0X06,
  GOAWAY_FRAME = 0X07,
  WINDOWUP_DATE_FRAME = 0X08,
  CONTINUATION_FRAME = 0X09,
  INVALID_FRAME_TYPE = 0XFF,
};

enum class flag : u8 {
  ACK = 0x01,
  END_STREAM = 0x01,
  END_SEGMENT = 0x02,
  END_HEADERS = 0x04,
  PADDED = 0x08,
  PRIORITY = 0x20,
};

enum class setting : u16 {
  SETTINGS_HEADER_TABLE_SIZE = 0x1,
  SETTINGS_ENABLE_PUSH = 0x2,
  SETTINGS_MAX_CONCURRENT_STREAMS = 0x3,
  SETTINGS_INITIAL_WINDOW_SIZE = 0x4,
  SETTINGS_MAX_FRAME_SIZE = 0x5,
  SETTINGS_MAX_HEADER_LIST_SIZE = 0x6,
  SETTINGS_MAX
};

enum class error : u8 {
  NO_ERROR = 0x0,       // The associated condition is not a result of an error.
  PROTOCOL_ERROR = 0x1, // The endpoint detected an unspecific protocol error.
  INTERNAL_ERROR = 0x2, // The endpoint encountered an unexpected internal
                        // error.
  FLOW_CONTROL_ERROR = 0x3, // The endpoint detected that its peer violated the
                            // flow-control protocol.
  SETTINGS_TIMEOUT = 0x4,   // The endpoint sent a SETTINGS frame but did not
                            // receive a response in a timely manner.
  STREAM_CLOSED = 0x5,      // The endpoint received a frame after a stream was
                            // half-closed.
  FRAME_SIZE_ERROR = 0x6, // The endpoint received a frame with an invalid size.
  REFUSED_STREAM = 0x7,   // The endpoint refused the stream prior to performing
                          // any application processing.
  CANCEL = 0x8, // Used by the endpoint to indicate that the stream is no longer
                // needed.
  COMPRESSION_ERROR = 0x9, // The endpoint is unable to maintain the header
                           // compression context for the connection.
  CONNECT_ERROR = 0xa, // The connection established in response to a CONNECT
                       // request (Section 8.3) was reset or abnormally closed.
  ENHANCE_YOUR_CALM = 0xb, // The endpoint detected that its peer is exhibiting
                           // a behavior that might be generating excessive
                           // load.
  INADEQUATE_SECURITY = 0xc, // The underlying transport has properties that do
                             // not meet minimum security requirements (see
                             // Section 9.2).
  HTTP_1_1_REQUIRED = 0xd,   // The endpoint requires that HTTP/1.1 be used
                             // instead of HTTP/2.
};

enum : u32 {
  FRAME_HEADER_SIZE = 9,
  CONNECTION_PREFACE_LENGTH = sizeof(INITIATION_STRING) - 1,
  MAX_ALLOWED_FRAME_SIZE = (1u << 24) - 1,
  MAX_ALLOWED_WINDOW_SIZE = (1u << 31) - 1,
};
static_assert(CONNECTION_PREFACE_LENGTH == 24, "");
};

struct http2_delegate {
  virtual ~http2_delegate() = default;
  virtual void setting_received(u16 id, u32 value) = 0;
  virtual void settings_end() = 0;
  virtual void send_connection_error(http2::error e) = 0;
};

using namespace http2;

class http2_frame_decoder {
  u32 _length = 0;
  frame _type = frame::INVALID_FRAME_TYPE;
  u8 _flags = 0;
  u8 _padding = 0;
  u32 _stream_id = 0;
  mut< http2_delegate > _delegate = nullptr;

public:
  http2_frame_decoder(mut< http2_delegate > delegate) : _delegate(delegate) {
    assert(_delegate != nullptr);
  }

  void decode(mut< buffer > in) {
    while (in->size() > 0) {
      std::cout << in->size() << std::endl;
      switch (_state) {
        case http2::state::READ_CONNECTION_PREFACE:
          read_connection_preface(in);
          break;
        case http2::state::READ_FRAME_HEADER:
          read_connection_header(in);
          break;
        case http2::state::READ_FRAME_PADDING:
          read_frame_padding(in);
          break;
        case http2::state::READ_DATA_FRAME:
          read_data_frame(in);
          break;
        case http2::state::READ_HEADERS_FRAME:
          std::cout << "Reading HEADERS" << std::endl;
          in->skip_bytes(_length);
          _state = http2::state::READ_FRAME_HEADER;
          break;
        case http2::state::READ_PRIORITY_FRAME:
          std::cout << "Reading PRIORITY" << std::endl;
          in->skip_bytes(_length);
          _state = http2::state::READ_FRAME_HEADER;
          break;
        case http2::state::READ_RST_STREAM_FRAME:
          std::cout << "Reading RST_STREAM" << std::endl;
          in->skip_bytes(_length);
          _state = http2::state::READ_FRAME_HEADER;
          break;
        case http2::state::READ_SETTINGS_FRAME:
          std::cout << "Reading SETTINGS" << std::endl;
          read_settings_frame(in);
          _state = http2::state::READ_FRAME_HEADER;
          break;
        case http2::state::READ_PUSH_PROMISE_FRAME:
          std::cout << "Reading PUSH_PROMISE" << std::endl;
          in->skip_bytes(_length);
          _state = http2::state::READ_FRAME_HEADER;
          break;
        case http2::state::READ_PING_FRAME:
          std::cout << "Reading PING" << std::endl;
          in->skip_bytes(_length);
          _state = http2::state::READ_FRAME_HEADER;
          break;
        case http2::state::READ_GOAWAY_FRAME:
          std::cout << "Reading GOAWAY" << std::endl;
          in->skip_bytes(_length);
          _state = http2::state::READ_FRAME_HEADER;
          break;
        case http2::state::READ_WINDOW_UPDATE_FRAME:
          std::cout << "Reading WINDOW_UPDATE" << std::endl;
          in->skip_bytes(_length);
          _state = http2::state::READ_FRAME_HEADER;
          break;
        case http2::state::READ_CONTINUATION_FRAME:
          std::cout << "Reading CONTINUATION" << std::endl;
          in->skip_bytes(_length);
          _state = http2::state::READ_FRAME_HEADER;
          break;
        case http2::state::CONNECTION_ERROR:
          std::cout << "Connection error" << std::endl;
          return;
          break;
      }
    }
  }

private:
  void read_connection_preface(mut< buffer > in) {
    while (auto b = in->read< char >()) {
      if (b.unwrap() != INITIATION_STRING[_length++]) {
        _state = http2::state::CONNECTION_ERROR;
        _delegate->send_connection_error(http2::error::PROTOCOL_ERROR);
        return;
      }
      if (_length == CONNECTION_PREFACE_LENGTH) {
        std::cout << "HTTP2 Yay!" << std::endl;
        _state = http2::state::READ_FRAME_HEADER;
        return;
      }
    }
  }

  void read_frame_padding(mut< buffer > in) {
    if (auto p = in->read< u8 >()) {
      _padding = p.unwrap();
      _state = get_state_from_type();
      --_length;
    }
  }

  void read_settings_frame(mut< buffer > in) {
    auto ack = is_flag_set(flag::ACK);
    if (_length == 0) {
      _state = http2::state::READ_FRAME_HEADER;
      return;
    }
    if (ack) {
      return transition_to_error(http2::error::FRAME_SIZE_ERROR);
    }
    if (_length % 6) {
      return transition_to_error(http2::error::PROTOCOL_ERROR);
    }
    if (_stream_id) {
      return transition_to_error(http2::error::PROTOCOL_ERROR);
    }
    while (_length && in->size() >= 6) {
      auto id = read_u16(in);
      auto value = read_u32(in);
      std::cout << "id:" << id << " value:" << value << std::endl;
      if (id > (u16)setting::SETTINGS_MAX) {
        std::cout << "Ignoring unknown setting" << std::endl;
        continue;
      }
      switch ((setting)id) {
        case setting::SETTINGS_ENABLE_PUSH:
          if (value > 1) {
            return transition_to_error(http2::error::PROTOCOL_ERROR);
          }
          break;
        case setting::SETTINGS_INITIAL_WINDOW_SIZE:
          if (value > MAX_ALLOWED_WINDOW_SIZE) {
            return transition_to_error(http2::error::FLOW_CONTROL_ERROR);
          }
          break;
        case setting::SETTINGS_MAX_FRAME_SIZE:
          if (value > MAX_ALLOWED_FRAME_SIZE) {
            return transition_to_error(http2::error::PROTOCOL_ERROR);
          }
          break;
        default:
          break;
      }
      _delegate->setting_received(id, value);
      _length -= 6;
    }
    if (0 == _length) {
      _delegate->settings_end();
    }
  }

  void read_data_frame(mut< buffer > in) {
    std::cout << "Reading DATA" << std::endl;
    in->skip_bytes(_length);
    _state = http2::state::READ_FRAME_HEADER;
  }

  u16 read_u16(mut< buffer > in) {
    struct _u16 {
      u8 bytes[2];
    } val = in->read< _u16 >().unwrap();

    return val.bytes[0] << 8 | val.bytes[1];
  }

  u32 read_u24(mut< buffer > in) {
    struct _u24 {
      u8 bytes[3];
    } val = in->read< _u24 >().unwrap();

    return val.bytes[0] << 16 | val.bytes[1] << 8 | val.bytes[2];
  }

  u32 read_u32(mut< buffer > in) {
    struct _u32 {
      u8 bytes[4];
    } val = in->read< _u32 >().unwrap();

    return val.bytes[0] << 24 | val.bytes[1] << 16 | val.bytes[2] << 8 |
           val.bytes[3];
  }

  void read_connection_header(mut< buffer > in) {
    if (in->size() >= FRAME_HEADER_SIZE) {
      _length = read_u24(in);
      std::cout << "Length: " << _length << std::endl;
      _type = in->read< frame >().unwrap();
      std::cout << "Type: " << (int)_type << std::endl;
      _flags = in->read< u8 >().unwrap();
      std::cout << "Flags: " << (int)_flags << std::endl;
      _stream_id = read_u32(in);
      std::cout << "Stream: " << _stream_id << std::endl;

      _state = is_flag_set(flag::PADDED) ? http2::state::READ_FRAME_PADDING
                                         : get_state_from_type();
    }
  }
  bool is_flag_set(flag f) {
    return _flags & static_cast< u8 >(f);
  }
  http2::state get_state_from_type() {
    return static_cast< http2::state >(static_cast< u8 >(_type) | 0xF0);
  }
  void transition_to_error(http2::error e) {
    _state = http2::state::CONNECTION_ERROR;
    _delegate->send_connection_error(e);
  }
};

class http2_codec : public pipes::context_aware_filter< mut< buffer > >,
                    public http2_delegate {

  http2_frame_decoder _decoder{this};
  own< buffer_allocator > _alloc;
  struct remote_settings {
    u32 header_table_size = 4096;
    bool enable_push = true;
    u32 max_concurrent_streams = 100;
    u32 initial_window_size = (1 << 16) - 1;
    u32 max_frame_size = 1 << 14;
    u32 max_header_list_size = -1;
  } _remote_settings;

public:
  http2_codec(own< buffer_allocator > alloc) : _alloc(move(alloc)) {
  }

  void read(mut< context > cx, mut< buffer > in) final override {
    _decoder.decode(in);
    cx->pipe()->push_front(make<range_echo>());
  }

  void setting_received(u16 id, u32 value) override {
    std::cout << "id:" << id << " value:" << value << std::endl;
    switch ((setting)id) {
      case setting::SETTINGS_HEADER_TABLE_SIZE:
        _remote_settings.header_table_size = value;
        break;
      case setting::SETTINGS_ENABLE_PUSH:
        _remote_settings.enable_push = (bool)value;
        break;
      case setting::SETTINGS_MAX_CONCURRENT_STREAMS:
        _remote_settings.max_concurrent_streams = value;
        break;
      case setting::SETTINGS_INITIAL_WINDOW_SIZE:
        _remote_settings.initial_window_size = value;
        break;
      case setting::SETTINGS_MAX_FRAME_SIZE:
        _remote_settings.max_frame_size = value;
        break;
      case setting::SETTINGS_MAX_HEADER_LIST_SIZE:
        _remote_settings.max_header_list_size = value;
        break;
      default:
        break;
    }
  }
  void settings_end() override {
    auto b = _alloc->allocate(1 << 10);
    b.write(byte_range{"\0\0\0\4\1\0\0\0\0"});
    my_context()->forward_write(edit(b));
  }
  void send_connection_error(http2::error e) override {
    auto b = _alloc->allocate(1 << 10);
    b.write(byte_range{"\0\0\x8\7\0\0\0\0\0\0\0\0\0\0\0\0"});
    b.write(byte_range{e});
    my_context()->forward_write(edit(b));
    my_context()->channel()->close();
  }
};

class http2_channel : public net::client_channel< net::kInet, net::kTCP > {
public:
  template < class... A >
  http2_channel(A&&... args)
      : client_channel(forward< A >(args)...) {
    pipe()->push_back(make< logging_filter >());
    // pipe()->push_back(make< range_echo >());
    pipe()->push_back(make< http2_codec >(alloc()));
  }
};

using reactive_service = xi::async::sharded_service<
    xi::async::reactor_service< libevent::reactor > >;

template < net::address_family af, net::protocol p >
class channel_initialize_filter
    : public pipes::filter<
          pipes::in< own< net::client_channel< net::kInet, net::kTCP > > > > {
  mut< reactive_service > _reactive_service;
  mut< core::executor_pool > _pool;

protected:
  using channel = net::client_channel< af, p >;
  using base = channel_initialize_filter;

  void read(mut< context > cx, own< channel > ch) final override {
    _pool->post([ this, rs = _reactive_service, ch = move(ch) ]() mutable {
      initialize_channel(edit(ch));
      rs->local()->reactor()->attach_handler(move(ch));
    });
  }

public:
  channel_initialize_filter(mut< reactive_service > rs,
                            mut< core::executor_pool > pool)
      : _reactive_service(rs), _pool(pool) {
  }

  virtual ~channel_initialize_filter() = default;
  virtual void initialize_channel(mut< channel > ch) = 0;
};

struct http2_channel_factory
    : public net::channel_factory< net::kInet, net::kTCP > {
  own< channel_t > create_channel(net::stream_client_socket s) override {
    return make< http2_channel >(move(s));
  }
};

class http2_handler
    : public channel_initialize_filter< net::kInet, net::kTCP > {
public:
  using base::base;

  void initialize_channel(mut< channel > ch) override {
    // ch->pipe()->push_back(make< logging_filter >());
    // ch->pipe()->push_back(make< fixed_length_frame_decoder >(1 << 12));
    // ch->pipe()->push_back(make< http2_frame_decoder >());
    // ch->pipe()->push_back(make< range_echo >());
  }
};

auto k = make< core::launchable_kernel< core::thread_launcher > >();

int main(int argc, char* argv[]) {

  struct sigaction SIGINT_action;
  SIGINT_action.sa_handler = [](int sig) { k->initiate_shutdown(); };
  sigemptyset(&SIGINT_action.sa_mask);
  SIGINT_action.sa_flags = 0;
  sigaction(SIGINT, &SIGINT_action, nullptr);

  sigblock(sigmask(SIGPIPE));
  k->start(1, 1 << 20);
  auto pool = make_executor_pool(edit(k));

  auto r_service = make< reactive_service >(share(pool));
  r_service->start().then([=, &pool, &r_service] {
    auto acc = make< net::acceptor< net::kInet, net::kTCP > >();

    std::cout << "Acceptor created." << std::endl;
    try {
      acc->bind(argc > 1 ? atoi(argv[1]) : 19999);
    } catch (ref< std::exception > e) {
      std::cout << "Bind error: " << e.what() << std::endl;
      exit(1);
    }
    std::cout << "Acceptor bound." << std::endl;
    acc->set_channel_factory(make< http2_channel_factory >());
    acc->pipe()->push_back(make< http2_handler >(edit(r_service), edit(pool)));
    acc->set_child_option(net::option::tcp::no_delay::yes);

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
