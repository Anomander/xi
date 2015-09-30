#include "xi/io/async_channel.h"
#include "xi/async/libevent/reactor.h"
#include "xi/hw/hardware.h"
#include "xi/core/launchable_kernel.h"
#include "xi/core/thread_launcher.h"
#include "xi/core/kernel_utils.h"
#include "xi/async/reactor_service.h"

using ::boost::intrusive_ptr;
using namespace xi;
using namespace xi::async;
using namespace xi::async::libevent;
using namespace xi::io;

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

struct buf_base : public ownership::rc_shared {
  uint8_t *_data = nullptr;
  size_t _size = 0;
  size_t _consumed = 0;

public:
  buf_base(uint8_t *data, size_t sz) : _data(data), _size(sz) {}
  uint8_t *data() { return _data; }
  size_t consumed() const { return _consumed; }
  size_t consume(size_t length) { return _consumed += length; }
  size_t size() const { return _size; }
  size_t remaining() const { return _size - _consumed; }
};

struct byte_range_base {
  own< buf_base > _buf;
  uint32_t _start_offset = 0;
  uint32_t _length = 0;

  byte_range_base() = default;
  byte_range_base(own< buf_base > buf, size_t offset, size_t length)
      : _buf(move(buf)), _start_offset(offset), _length(length) {
    // std::cout << "Created range of " << _length << " bytes in ["
    //           << (void *)(_buf->data() + _start_offset) << "-"
    //           << (void *)(_buf->data() + _start_offset + _length) << "]"
    //           << std::endl;
  }
};

class const_byte_range : protected byte_range_base {
public:
  const_byte_range() = default;
  const_byte_range(own< buf_base > buf, size_t offset, size_t length)
      : byte_range_base(move(buf), offset, length) {}
  const_byte_range(const_byte_range const &) = default;
  const_byte_range &operator=(const_byte_range const &) = default;
  const_byte_range(const_byte_range &&) = default;
  const_byte_range &operator=(const_byte_range &&) = default;

  const uint8_t *readable_bytes() const { return _buf->data() + _start_offset; }
  const uint8_t *cbegin() const { return readable_bytes(); }
  const uint8_t *cend() const { return cbegin() + _length; }
  uint32_t length() const { return _length; }
  bool is_empty() const { return 0 == _length; }
};

class mutable_byte_range : public const_byte_range {
public:
  mutable_byte_range() = default;
  mutable_byte_range(own< buf_base > buf, size_t offset, size_t length)
      : const_byte_range(move(buf), offset, length) {}

  mutable_byte_range(mutable_byte_range const &) = delete;
  mutable_byte_range &operator=(mutable_byte_range const &) = delete;
  mutable_byte_range(mutable_byte_range &&) = default;
  mutable_byte_range &operator=(mutable_byte_range &&) = default;

  uint8_t *writable_bytes() { return _buf->data() + _start_offset; }
  uint8_t *begin() { return writable_bytes(); }
  uint8_t *end() { return begin() + _length; }
  mutable_byte_range split(size_t split_point) {
    if (split_point >= _length)
      return {share(_buf), _start_offset + _length, 0};
    XI_SCOPE(exit) { _length = split_point; };
    return {share(_buf), _start_offset + split_point, _length - split_point};
  }
};

class buf : public buf_base {
public:
  using buf_base::buf_base;
  opt< mutable_byte_range > allocate_range(size_t length) {
    if (remaining() < length) { return none; }
    auto old_consumed = consumed();
    consume(length);
    return some(mutable_byte_range{share(this), old_consumed, length});
  }
};

class heap_buf : public buf {
public:
  heap_buf(size_t sz) : buf(new uint8_t[sz], sz) {
    std::cout << "Allocated " << sz << " bytes in [" << (void *)_data << "-"
              << (void *)(_data + _size) << "]" << std::endl;
  }
  ~heap_buf() {
    delete[] data();
    std::cout << "Deleted " << size() << " bytes";
  }
};

class range_buf : public buf {
  mutable_byte_range _range;

public:
  range_buf(mutable_byte_range rg)
      : buf(rg.writable_bytes(), rg.length()), _range(move(rg)) {}
  ~range_buf() = default;
};

struct range_allocator : public virtual ownership::std_shared {
  virtual ~range_allocator() = default;
  virtual mutable_byte_range allocate(size_t size) = 0;
  virtual mutable_byte_range reallocate(mutable_byte_range &&, size_t size) = 0;
};

class paged_allocator : public range_allocator {
  own< heap_buf > _current_buffer;
  size_t _page_size;

public:
  paged_allocator(size_t page_size) : _page_size(page_size) {}

public:
  mutable_byte_range allocate(size_t size) override {
    validate(size);
    return _current_buffer->allocate_range(size).unwrap_or([&, this]() mutable {
      _current_buffer = make< heap_buf >(_page_size);
      return _current_buffer->allocate_range(size).unwrap();
    });
  }
  mutable_byte_range reallocate(mutable_byte_range &&rg, size_t size) override {
    if (size <= rg.length()) return move(rg);
    validate(size);
    auto range = allocate(size);
    copy(begin(rg), end(rg), begin(range));
    return move(range);
  }

private:
  void validate(size_t size) {
    if (size > _page_size) { throw std::bad_alloc(); }
    if (!is_valid(_current_buffer)) {
      _current_buffer = make< heap_buf >(_page_size);
    }
  }
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

class fixed_stream_reader
    : public pipes::filter< io::socket_readable, error_code,
                            const_byte_range > {
  size_t _read_amount;
  mut< client_channel2< kInet, kTCP > > _channel;
  own< range_allocator > _alloc;
  mutable_byte_range _range;

public:
  fixed_stream_reader(size_t read_amount,
                      mut< client_channel2< kInet, kTCP > > channel,
                      own< range_allocator > alloc)
      : _read_amount(read_amount), _channel(channel), _alloc(move(alloc)) {}

  void read(mut< context > cx, io::socket_readable) override {
    // std::cout << "Thread " << pthread_self() << " uses allocator "
    //           << address_of(_alloc) << std::endl;
    if (_range.is_empty()) { _range = _alloc->allocate(_read_amount); }
    auto read =
        _channel->read(byte_range{_range.writable_bytes(), _range.length()});
    if (read.has_error()) {
      static const error_code EAgain =
          make_error_code(std::errc::resource_unavailable_try_again);
      static const error_code EWould_block =
          make_error_code(std::errc::operation_would_block);

      auto error = read.error();
      if (error != EAgain && error != EWould_block) { cx->forward_read(error); }
      return;
    }
    auto read_range = move(_range);
    _range = read_range.split(read);
    cx->forward_read(move(read_range));
  }

  void read(mut<context>cx, error_code error)override{
    if (error == io::error::kEOF) {
      std::cout << "Channel closed by remote peer" << std::endl;
    } else {
      std::cout << "Channel error: " << error.message()
                << std::endl;
    }
    _channel->close();
  }

  void write(mut< context > cx, const_byte_range rg) override {
    _channel->write(byte_range{(uint8_t *)rg.readable_bytes(), rg.length()});
  }
};

class range_echo : public pipes::filter< const_byte_range > {
public:
  void read(mut< context > cx, const_byte_range rg) override {
    cx->forward_write(move(rg));
  }
};

class stream_rpc_data_read_filter
    : public pipes::filter< intrusive_ptr< protocol_message >,
                            io::socket_readable, error_code > {
  rpc_stream_reader _reader;
  mut< client_channel2< kInet, kTCP > > _channel;

public:
  stream_rpc_data_read_filter(mut< client_channel2< kInet, kTCP > > channel)
      : _reader(channel), _channel(channel) {}

  // void write(mut<Context>cx, intrusive_ptr<Protocol_message> msg) override
  // {
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

static thread_local own< range_allocator > ALLOC =
    make< paged_allocator >(1 << 28);

using reactive_service =
    xi::async::service< xi::async::reactor_service< libevent::reactor > >;

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
