#include "io/AsyncChannel.h"
#include "io/pipeline/Util.h"
#include "async/libevent/Reactor.h"
#include "hw/Hardware.h"
#include "core/LaunchableKernel.h"
#include "core/ThreadLauncher.h"
#include "core/KernelUtils.h"
#include "async/ReactorService.h"

using namespace xi;
using namespace xi::async;
using namespace xi::async::libevent;
using namespace xi::io;

class DataHandler : public pipeline::SimpleInboundPipelineHandler< DataAvailable > {
public:
  void messageReceived(mut< pipeline::HandlerContext > cx, own< DataAvailable > msg) override {
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

      auto bytesRead = static_cast< size_t >(read);
      // std::cout << "I just read " << bytesRead << " bytes." << std::endl;
      if (_messageCursor) {
        if (bytesRead >= _remainingSize) {
          // std::cout << "Complete message" << std::endl;
          auto* message = _currentMessage;
          _messageCursor = _currentMessage = nullptr;
          auto protocolMessage = new (message - sizeof(ProtocolHeader)) ProtocolMessage;
          cx->forward(pipeline::MessageRead{make< DataMessage >(protocolMessage)});
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
          auto space = (uint8_t*)::malloc(_remainingSize + sizeof(ProtocolHeader));
          ::memcpy(space, hdr, sizeof(ProtocolHeader));
          _messageCursor = _currentMessage = space + sizeof(ProtocolHeader);
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

using ReactiveService = async::Service< async::ReactorService< libevent::Reactor > >;

decltype(auto) localReactor() { return local< libevent::Reactor >(); }

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
  reactiveService->start().then([&pool] {
    auto ch = make< ServerChannel< kInet, kTCP > >();

    ch->bind(19999);
    ch->pipeline()->pushBack(pipeline::makeInboundHandler< ClientChannelConnected >([&](auto cx, auto msg) {
      pool->post([ch = msg->extractChannel()] {
        auto& reactor = localReactor();
        ch->pipeline()->pushBack(make< DataHandler >());
        ch->pipeline()->pushBack(make< MessageHandler >());
        std::cout << ch.use_count() << std::endl;
        reactor.attachHandler(move(ch));
        std::cout << "Local reactor @ " << addressOf(reactor) << std::endl;
      });
    }));

    pool->post([&, ch = move(ch) ] {
      std::cout << "reading reactor in: " << pthread_self() << std::endl;
      auto& reactor = localReactor();
      reactor.attachHandler(move(ch));
      std::cout << "Acceptor attached." << std::endl;
    });
  });

  k->awaitShutdown();
}
