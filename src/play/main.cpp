#include "io/AsyncChannel.h"
#include "io/pipeline/Util.h"
#include "async/libevent/Reactor.h"
#include "async/Engine.h"
#include "async/ThreadExecutor.h"
#include "hw/Hardware.h"
#include "core/LaunchableKernel.h"
#include "core/ThreadLauncher.h"
#include "core/KernelUtils.h"

using namespace xi;
using namespace xi::async;
using namespace xi::async::libevent;
using namespace xi::io;

class DataHandler : public pipeline::SimpleInboundPipelineHandler< DataAvailable > {
public:
  void messageReceived(mut< pipeline::HandlerContext > cx, own< DataAvailable > msg) override {
    std::cout << "Message received." << std::endl;
    Expected< int > read = 0;
    while (true) {
      if (nullptr != _messageCursor) {
        read = cx->channel()->read({ByteRange{_messageCursor, _remainingSize}, headerByteRange()});
      } else {
        read = cx->channel()->read({headerByteRange()});
      }
      if (read.hasError()) {
        auto error = read.error();
        if (error == make_error_code(SystemError::resource_unavailable_try_again)) {
          break;
        }
        std::cout << "Error " << error.message() << std::endl;
        cx->fire(pipeline::ChannelError(error));
        return;
      }

      std::cout << "I just read " << read << " bytes." << std::endl;
      auto bytesRead = static_cast< size_t >(read);
      if (_messageCursor) {
        if (bytesRead >= _remainingSize) {
          std::cout << "Complete message" << std::endl;
          auto* message = _currentMessage;
          _messageCursor = _currentMessage = nullptr;
          cx->forward(pipeline::MessageRead{make< DataMessage >(new (message) ProtocolMessage)});
          bytesRead -= _remainingSize;
          _remainingSize = 0;
        } else {
          _messageCursor += bytesRead;
          _remainingSize -= bytesRead;
          bytesRead = 0;
        }
        std::cout << _remainingSize << " bytes left to read." << std::endl;
      }
      if (bytesRead > 0) {
        _headerCursor += bytesRead;
        if (_headerCursor == end(_headerBytes)) {
          std::cout << "New header" << std::endl;
          auto* hdr = reinterpret_cast< ProtocolHeader* >(_headerBytes);
          _remainingSize = hdr->size;
          _messageCursor = _currentMessage = (uint8_t*)::malloc(_remainingSize);
          _headerCursor = begin(_headerBytes);
        }
      }
    }
    // cx->channel()->write(move(msg));
  }

private:
  ByteRange headerByteRange() const noexcept {
    return ByteRange{_headerCursor, sizeof(ProtocolHeader) - (_headerCursor - begin(_headerBytes))};
  }

private:
  uint8_t _headerBytes[sizeof(ProtocolHeader)];
  uint8_t* _currentMessage = nullptr;
  uint8_t* _messageCursor = nullptr;
  uint8_t* _headerCursor = _headerBytes;
  size_t _remainingSize = 0UL;
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

int main(int argc, char* argv[]) {
  auto k = make< core::LaunchableKernel< core::ThreadLauncher > >();
  k->start(4, 1 * 1024);
  auto pool = makeExecutorPool(edit(k), {0, 1, 2, 3});

  pool->postOnAll(ForkBomb(edit(pool)));
  // pool->postOnAll(ForkBombTask(edit(pool)));
  // pool->postOnAll(ForkBloat< 2024 >(edit(pool)));
  // pool->postOnAll([p = share(pool), k=edit(k)] {
  //   std::cout << "Hello, world!" << std::endl;
  //   p->postOnAll([p = share(p)] { std::cout << "Hello, world!" << std::endl; });
  //   // k->initiateShutdown();
  // });
  k->awaitShutdown();
  return 0;

  auto& engine = local< Engine< ThreadExecutor, libevent::Reactor > >();
  auto ch = make< ServerChannel< kInet, kTCP > >();

  ch->bind(19999);
  ch->pipeline()->pushBack(pipeline::makeInboundHandler< ClientChannelConnected >([&](auto cx, auto msg) {
    engine.runOnNext([ch = msg->extractChannel()] {
      auto& reactor = local< libevent::Reactor >();
      ch->pipeline()->pushBack(make< DataHandler >());
      ch->pipeline()->pushBack(make< MessageHandler >());
      std::cout << ch.use_count() << std::endl;
      reactor.attachHandler(move(ch));
      std::cout << "Local reactor @ " << &reactor << std::endl;
      std::cout << "Channel attached, exec id " << local< Executor >().id() << std::endl;
    });
  }));

  engine.start().then([&, ch = move(ch) ] {
    auto& reactor = local< libevent::Reactor >();
    reactor.attachHandler(move(ch));
    std::cout << "Local reactor @ " << &reactor << std::endl;
    std::cout << "Acceptor attached." << std::endl;
  });

  std::cout << "Must be running now" << std::endl;
  engine.join();
}
