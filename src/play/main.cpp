#define XI_USER_UPSTREAM_MESSAGE_ROOT boost::intrusive_ptr< ProtocolMessage >

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include "xi/io/AsyncChannel.h"
#include "xi/io/pipeline/Util.h"
#include "xi/async/libevent/Reactor.h"
#include "xi/hw/Hardware.h"
#include "xi/core/LaunchableKernel.h"
#include "xi/core/ThreadLauncher.h"
#include "xi/core/KernelUtils.h"
#include "xi/async/ReactorService.h"

#include <malloc.h>

using ::boost::intrusive_ptr;
using namespace xi;
using namespace xi::async;
using namespace xi::async::libevent;
using namespace xi::io;

class DataHandler : public pipeline::PipelineHandler {
public:
  void handleEvent(mut< pipeline::HandlerContext > cx, pipeline::DataAvailable msg) override {
    // std::cout << "Message received." << std::endl;
    Expected< int > read = 0;
    while (true) {
      if (nullptr != _messageCursor) {
        // std::cout << "Cursor:" << (void*)_messageCursor << " size:" << _remainingSize << std::endl;
        read = cx->channel()->read(
            {ByteRange{_messageCursor, _remainingSize}, ByteRange{_headerBytes, sizeof(ProtocolHeader)}});
      } else {
        read = cx->channel()->read({headerByteRange()});
      }

      if (cx->channel()->isClosed()) {
        break;
      }
      auto bytesRead = static_cast< size_t >(read);
      // std::cout << "I just read " << bytesRead << " bytes." << std::endl;
      if (_messageCursor) {
        if (bytesRead >= _remainingSize) {
          // std::cout << "Complete message" << std::endl;
          auto* protocolMessage = _currentMessage;
          _messageCursor = nullptr;
          _currentMessage = nullptr;
          // auto protocolMessage = new (message - sizeof(ProtocolHeader)) ProtocolMessage;
          // cx->forward(pipeline::MessageRead{make< DataMessage >(protocolMessage)});
          cx->forward(pipeline::UserUpstreamEvent{intrusive_ptr< ProtocolMessage >(protocolMessage)});
          bytesRead -= _remainingSize;
          _remainingSize = 0;
          _headerCursor = _headerBytes;
        } else {
          _messageCursor += bytesRead;
          _remainingSize -= bytesRead;
          bytesRead = 0;
        }
        // std::cout << _remainingSize << " bytes left to read." << std::endl;
      }
      // std::cout << bytesRead << " unconsumed bytes left." << std::endl;
      if (bytesRead > 0) {
        // for (size_t i = 0; i < bytesRead; ++i) {
        //   std::cout << (int)*(_headerCursor + i) << " ";
        // }
        // std::cout << std::endl;
        // std::cout << "Header cursor: " << (void*)_headerCursor << std::endl;
        // std::cout << "Header bytes: " << (void*)_headerBytes << std::endl;
        _headerCursor += bytesRead;
        if (_headerCursor == end(_headerBytes)) {
          // std::cout << "New header" << std::endl;
          auto* hdr = reinterpret_cast< ProtocolHeader* >(_headerBytes);

          // std::cout << "size: " << hdr->size << std::endl;
          // std::cout << "version: " << hdr->version << std::endl;
          // std::cout << "type: " << hdr->type << std::endl;

          _remainingSize = hdr->size;
          auto message = (ProtocolMessage*)::malloc(_remainingSize + sizeof(ProtocolMessage));
          ::memcpy(&(message->_header), hdr, sizeof(ProtocolHeader));
          _currentMessage = message;
          _messageCursor = message->_data;
          _headerCursor = begin(_headerBytes);
        }
      }
    }
  }

private:
  ByteRange headerByteRange() const noexcept {
    return ByteRange{_headerCursor, sizeof(ProtocolHeader) - (_headerCursor - begin(_headerBytes))};
  }

private:
  uint8_t _headerBytes[sizeof(ProtocolHeader)];
  ProtocolMessage* _currentMessage = nullptr;
  uint8_t* _messageCursor = nullptr;
  uint8_t* _headerCursor = _headerBytes;
  size_t _remainingSize = 0UL;
};

class UserMessageHandler : public pipeline::PipelineHandler {
protected:
#ifdef XI_USER_UPSTREAM_MESSAGE_ROOT
  void handleEvent(mut< pipeline::HandlerContext > cx, pipeline::UserUpstreamEvent event) final override {
    upstreamMessage(cx, event.root);
  }
#endif // XI_USER_UPSTREAM_MESSAGE_ROOT
#ifdef XI_USER_DOWNSTREAM_MESSAGE_ROOT
  void handleEvent(mut< pipeline::HandlerContext > cx, pipeline::UserDownstreamEvent event) final override {
    downstreamMessage(cx, event.root);
  }
#endif // XI_USER_DOWNSTREAM_MESSAGE_ROOT
public:
#ifdef XI_USER_UPSTREAM_MESSAGE_ROOT
  virtual void upstreamMessage(mut< pipeline::HandlerContext > cx, XI_USER_UPSTREAM_MESSAGE_ROOT msg) = 0;
#endif // XI_USER_UPSTREAM_MESSAGE_ROOT
#ifdef XI_USER_DOWNSTREAM_MESSAGE_ROOT
  virtual void downstreamMessage(mut< pipeline::HandlerContext > cx, XI_USER_DOWNSTREAM_MESSAGE_ROOT msg) = 0;
#endif // XI_USER_DOWNSTREAM_MESSAGE_ROOT
};

class MyHandler : public UserMessageHandler {
public:
  void upstreamMessage(mut< pipeline::HandlerContext > cx, intrusive_ptr< ProtocolMessage > msg) override {
    // std::cout << "User event!" << std::endl;
    cx->channel()->write(ByteRange{(uint8_t*)&msg->_header, msg->header().size + sizeof(ProtocolHeader)});
  }
};

class MessageHandler : public pipeline::SimpleInboundPipelineHandler< DataMessage > {
public:
  void messageReceived(mut< pipeline::HandlerContext > cx, own< DataMessage > msg) override {
    // std::cout << "Message received." << std::endl;
    cx->channel()->write(move(msg));
  }
  void handleEvent(mut< pipeline::HandlerContext > cx, pipeline::ChannelError error) override {
    std::cout << "Channel error: " << error.error().message() << std::endl;
    cx->channel()->close();
  }
};

thread_local size_t Count;

struct ForkBombTask : public xi::core::Task {
  ForkBombTask(mut< xi::core::ExecutorPool > p) : _pool(p) {}
  void run() override {
    Count++;
    if (Count % 10000000 == 0) {
      std::cout << pthread_self() << " : " << Count << std::endl;
    }
    _pool->postOnAll(ForkBombTask(_pool));
  };

  mut< xi::core::ExecutorPool > _pool;
};

struct ForkBomb {
  ForkBomb(mut< xi::core::ExecutorPool > p) : _pool(p) {}
  void operator()() {
    Count++;
    if (Count % 10000000 == 0) {
      std::cout << pthread_self() << " : " << Count << std::endl;
    }
    _pool->post(ForkBomb(_pool));
    // _pool->postOnAll(ForkBomb(_pool));
  };

  mut< xi::core::ExecutorPool > _pool;
};

template < size_t N >
struct ForkBloat {
  ForkBloat(mut< xi::core::ExecutorPool > p) : _pool(p) {}
  void operator()() {
    std::cout << "ForkBloat running " << std::endl;
    Count++;
    if (Count % 10 == 0) {
      std::cout << pthread_self() << " : " << Count << std::endl;
    }
    _pool->postOnAll(ForkBloat< N >(_pool));
  };

  mut< xi::core::ExecutorPool > _pool;
  char bloat[N];
};

class RpcStreamReader {
  mut< ClientChannel2< kInet, kTCP > > _channel;
  uint8_t _headerBytes[sizeof(ProtocolHeader)];
  ProtocolMessage* _currentMessage = nullptr;
  uint8_t* _messageCursor = nullptr;
  uint8_t* _headerCursor = _headerBytes;
  size_t _remainingSize = 0UL;

private:
  ByteRange headerByteRange() const noexcept {
    return ByteRange{_headerCursor, sizeof(ProtocolHeader) - (_headerCursor - begin(_headerBytes))};
  }

public:
  RpcStreamReader(mut< ClientChannel2< kInet, kTCP > > channel) : _channel(channel) {}

  template < class FuncMsg, class FuncErr >
  void read(FuncMsg&& onMessageCallback, FuncErr&& onErrorCallback) {
    Expected< int > read = 0;
    while (true) {
      if (nullptr != _messageCursor) {
        read = _channel->read(
            {ByteRange{_messageCursor, _remainingSize}, ByteRange{_headerBytes, sizeof(ProtocolHeader)}});
      } else {
        read = _channel->read({headerByteRange()});
      }

      if (read.hasError()) {
        static const error_code EAgain = make_error_code(SystemError::resource_unavailable_try_again);
        static const error_code EWouldBlock = make_error_code(SystemError::operation_would_block);

        auto error = read.error();
        if (error != EAgain && error != EWouldBlock) {
          onErrorCallback(error);
        }
        return;
      }
      if (read == 0) {
        return;
      }
      auto bytesRead = static_cast< size_t >(read);
      intrusive_ptr< ProtocolMessage > protocolMessage = nullptr;
      if (_messageCursor) {
        if (bytesRead >= _remainingSize) {
          protocolMessage = _currentMessage;
          _messageCursor = nullptr;
          _currentMessage = nullptr;
          bytesRead -= _remainingSize;
          _remainingSize = 0;
          _headerCursor = _headerBytes;
        } else {
          _messageCursor += bytesRead;
          _remainingSize -= bytesRead;
          bytesRead = 0;
        }
      }
      if (bytesRead > 0) {
        _headerCursor += bytesRead;
        if (_headerCursor == end(_headerBytes)) {
          auto* hdr = reinterpret_cast< ProtocolHeader* >(_headerBytes);

          _remainingSize = hdr->size;
          auto message = (ProtocolMessage*)::malloc(_remainingSize + sizeof(ProtocolMessage));
          ::memcpy(&(message->_header), hdr, sizeof(ProtocolHeader));
          _currentMessage = message;
          _messageCursor = message->_data;
          _headerCursor = begin(_headerBytes);
        }
      }
      if (protocolMessage) {
        onMessageCallback(protocolMessage);
        protocolMessage = nullptr;
      }
    }
  }
};

class Buf {
  uint8_t * _data;
  size_t _size;
public:
  Buf (size_t sz) {
    _data = (uint8_t*)::malloc(sz);
    _size = ::malloc_usable_size(_data);
  }
};

class StreamRpcDataReadFilter
    : public pipe2::Filter< intrusive_ptr< ProtocolMessage >, io::SocketReadable, error_code > {
  RpcStreamReader _reader;
  mut< ClientChannel2< kInet, kTCP > > _channel;

public:
  StreamRpcDataReadFilter(mut< ClientChannel2< kInet, kTCP > > channel) : _reader(channel), _channel(channel) {}

  // void write(mut<Context>cx, intrusive_ptr<ProtocolMessage> msg) override {
  //   _channel->write();
  // }
  void read(mut< Context > cx, io::SocketReadable) override {
    _reader.read([cx, this](auto msg) {
                   // cx->forwardRead(move(msg));
                   _channel->write(ByteRange{(uint8_t*)&msg->_header, msg->header().size + sizeof(ProtocolHeader)});
                 },
                 [cx, this](auto error) {
                   if (error == io::error::kEof) {
                     std::cout << "Channel closed by remote peer" << std::endl;
                   } else {
                     std::cout << "Channel error: " << error.message() << std::endl;
                     cx->forwardRead(error);
                   }
                   _channel->close();
                 });
  }
};

using ReactiveService = async::Service< async::ReactorService< libevent::Reactor > >;

class AcceptorHandler : public pipe2::Filter< pipe2::ReadOnly< own< ClientChannel2< kInet, kTCP > > > > {
  mut< ReactiveService > _reactiveService;

public:
  AcceptorHandler(mut< ReactiveService > rs) : _reactiveService(rs) {}

  void read(mut< Context > cx, own< ClientChannel2< kInet, kTCP > > ch) {
    std::cout << "Channel ACCEPTED!" << std::endl;
    ch->pipe()->pushBack(make< StreamRpcDataReadFilter >(edit(ch)));
    auto reactor = _reactiveService->local()->reactor();
    reactor->attachHandler(move(ch));
  }

  void read(mut< Context > cx, exception_ptr ex) {
    try {
      rethrow_exception(ex);
    } catch (std::exception & e) {
      std::cout << "Exception: " << e.what() << std::endl;
    } catch (...) {
      std::cout << "Undefined exception" << std::endl;
    }
  }
};

int main(int argc, char* argv[]) {
  auto k = make< core::LaunchableKernel< core::ThreadLauncher > >();
  k->start(4, 1 << 20);
  auto pool = makeExecutorPool(edit(k), {0, 1, 2, 3});
  // pool->postOnAll(ForkBomb(edit(pool)));
  // pool->postOnAll(ForkBombTask(edit(pool)));
  // pool->postOnAll(ForkBloat< 2024 >(edit(pool)));
  // pool->postOnAll([p = share(pool), k=edit(k)] {
  //   std::cout << "Hello, world!" << std::endl;
  //   p->postOnAll([p = share(p)] { std::cout << "Hello, world!" << std::endl; });
  //   // k->initiateShutdown();
  // });

  auto reactiveService = make< ReactiveService >(share(pool));
  reactiveService->start().then([&pool, &reactiveService] {
    auto ch = make< ServerChannel< kInet, kTCP > >();
    auto acc = make< Acceptor< kInet, kTCP > >();

    // ch->bind(19999);
    acc->bind(19990);
    acc->pipe()->pushBack(make< AcceptorHandler >(edit(reactiveService)));

    ch->childHandler(pool->wrap([&reactiveService](auto ch) {
      auto reactor = reactiveService->local()->reactor();
      ch->pipeline()->pushBack(make< DataHandler >());
      ch->pipeline()->pushBack(make< MyHandler >());
      reactor->attachHandler(move(ch));
      std::cout << "Local reactor @ " << addressOf(reactor) << std::endl;
    }));

    pool->post([&, ch = move(ch), acc = move(acc) ] {
      auto reactor = reactiveService->local()->reactor();
      reactor->attachHandler(move(ch));
      reactor->attachHandler(move(acc));
      std::cout << "Acceptor attached." << std::endl;
    });
  });

  k->awaitShutdown();
}
