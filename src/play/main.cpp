#include "io/AsyncChannel.h"
#include "io/pipeline/Util.h"
#include "async/libevent/Reactor.h"
#include "async/Engine.h"

#include <boost/thread.hpp>

using namespace xi;
using namespace xi::async;
using namespace xi::async2;
using namespace xi::async::libevent;
using namespace xi::io;

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

int main(int argc, char* argv[]) {
  auto& engine = Context::local< Engine< ThreadExecutor, libevent::Reactor > >();
  auto ch = make< ServerChannel< kInet, kTCP > >();
  ch->bind(19999);
  ch->pipeline()->pushBack(pipeline::makeInboundHandler< ClientChannelConnected >([&](auto cx, auto msg) {
    engine.runOnNext([ch = msg->extractChannel()] {
      auto& reactor = local< libevent::Reactor >();
      ch->pipeline()->pushBack(make< MessageHandler >());
      std::cout << ch.use_count() << std::endl;
      reactor.attachHandler(move(ch));
      std::cout << "Local reactor @ " << &reactor << std::endl;
      std::cout << "Channel attached, exec id " << local< async2::Executor >().id() << std::endl;
    });
  }));
  engine.run([&, ch = move(ch) ] {
    auto& reactor = local< libevent::Reactor >();
    reactor.attachHandler(move(ch));
    std::cout << "Local reactor @ " << &reactor << std::endl;
    std::cout << "Acceptor attached." << std::endl;
  });
}
