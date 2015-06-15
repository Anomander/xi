#include "async/libevent/Executor.h"
#include "io/AsyncChannel.h"

#include <boost/thread.hpp>

using namespace xi;
using namespace xi::async::libevent;
using namespace xi::io;

class MessageHandler
  : public pipeline::SimpleInboundPipelineFastCastHandler <DataMessage>
{
public:
  void messageReceived(mut<pipeline::HandlerContext> cx, own <DataMessage> msg) override {
    // std::cout << "Message received." << std::endl;
    cx->channel()->write(move(msg));
  }
  void channelError (mut<pipeline::HandlerContext> cx, error_code error) override {
    std::cout << "Channel error: " << error.message() << std::endl;
    cx->channel()->close();
  }
};

int main() {
  Executor e;
  Executor child[4];
  int currentChild = 0;
  auto ch = make <ServerChannel<kInet, kTCP>>();
  ch->bind(19999);
  ch->childHandler ([&](auto ch) {
      // ch->expectWrite (false);
      ch->pipeline()->pushBack(make<MessageHandler>());
      child[currentChild = (currentChild + 1) % 4].attachHandler(move(ch));
      std::cout << "Connected" << std::endl;
    });
  e.attachHandler (move(ch));
  boost::thread_group g;
  for (auto & ch : child) {
    g.create_thread([&] { ch.run(); });
  }
  e.run();
  g.join_all();
}
