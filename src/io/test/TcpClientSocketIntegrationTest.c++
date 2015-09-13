#include "xi/io/AsyncChannel.h"
#include "xi/io/DataMessage.h"
#include "xi/async/libevent/Reactor.h"
#include "xi/io/pipeline/PipelineHandler.h"
#include "xi/io/pipeline/Util.h"

#include "TcpSocketMock.h"

#include <gtest/gtest.h>

using namespace xi;
using namespace xi::io;
using namespace xi::io::pipeline;
using namespace xi::async::libevent;

namespace xi {
namespace io {
  namespace pipeline {
    bool operator==(Event const& lhs, Event const& rhs) { return typeid(lhs) == typeid(rhs); }
    bool operator==(ChannelError const& lhs, ChannelError const& rhs) { return lhs.error() == rhs.error(); }
    std::ostream& operator<<(std::ostream& os, ChannelError const& rhs) { return os << typeid(rhs).name(); }
    std::ostream& operator<<(std::ostream& os, ChannelException const& rhs) { return os << typeid(rhs).name(); }
    std::ostream& operator<<(std::ostream& os, ChannelRegistered const& rhs) { return os << typeid(rhs).name(); }
    std::ostream& operator<<(std::ostream& os, ChannelDeregistered const& rhs) { return os << typeid(rhs).name(); }
    std::ostream& operator<<(std::ostream& os, ChannelClosed const& rhs) { return os << typeid(rhs).name(); }
    std::ostream& operator<<(std::ostream& os, DataAvailable const& rhs) { return os << typeid(rhs).name(); }
    std::ostream& operator<<(std::ostream& os, Event const& rhs) { return os << typeid(rhs).name(); }
  }
}
}

using AnyEvent =
    variant< ChannelClosed, ChannelRegistered, ChannelDeregistered, DataAvailable, ChannelError, ChannelException >;

class MessageHandler : public pipeline::PipelineHandler {
public:
  void handleEvent(mut< pipeline::HandlerContext > cx, pipeline::ChannelClosed e) override {
    events.emplace_back(e);
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    cx->forward(e);
  }
  void handleEvent(mut< pipeline::HandlerContext > cx, pipeline::ChannelRegistered e) override {
    events.emplace_back(e);
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    cx->forward(e);
  }
  void handleEvent(mut< pipeline::HandlerContext > cx, pipeline::ChannelDeregistered e) override {
    events.emplace_back(e);
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    cx->forward(e);
  }
  void handleEvent(mut< pipeline::HandlerContext > cx, pipeline::DataAvailable e) override {
    events.emplace_back(e);
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    sizeRead = cx->channel()->read(ByteRange{data, sizeof(data)});
    cx->forward(e);
  }
  void handleEvent(mut< pipeline::HandlerContext > cx, pipeline::ChannelError e) override {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    events.emplace_back(e);
    cx->forward(e);
  }
  void handleEvent(mut< pipeline::HandlerContext > cx, pipeline::ChannelException e) override {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    events.emplace_back(e);
    cx->forward(e);
  }

public:
  vector< AnyEvent > events;
  uint8_t data[1024];
  size_t sizeRead = 0;
};

uint8_t PAYLOAD[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

class TestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    reactor = make< Reactor >();
    handler = make< MessageHandler >();
    mock = make< test::TcpSocketMock >();
    auto ch = make< ServerChannel< kInet, kTCP > >();
    ch->setOption(ReuseAddress::yes);
    ch->bind(12345);
    ch->childHandler([&](auto channel) {
      channel->pipeline()->pushBack(handler);
      clientChannel = edit(channel);
      reactor->attachHandler(move(channel));
    });
    reactor->attachHandler(move(ch));

    mock.connect(12345);
    reactor->poll();
  }

  void verifyEventSequence(initializer_list< AnyEvent > events) {
    ASSERT_EQ(events.size(), handler->events.size());
    size_t i = 0;
    for (auto&& e : events) {
      ASSERT_EQ(e, handler->events[i++]);
    }
  }

  void verifyDataRead(uint8_t* data, size_t size) {
    ASSERT_EQ(size, handler->sizeRead);
    for (size_t i = 0; i < size; ++i) {
      ASSERT_EQ(data[i], handler->data[i]);
    }
  }

protected:
  own< Reactor > reactor;
  own< MessageHandler > handler;
  own< test::TcpSocketMock > mock;
  mut< AsyncChannel > clientChannel;
};

TEST_F(TestFixture, ClientInitiatedConnectSequence) {
  reactor->poll();

  verifyEventSequence({ChannelRegistered{}});
}

TEST_F(TestFixture, ClientInitiatedCloseSequence) {
  reactor->poll();

  verifyEventSequence({ChannelRegistered{}});

  mock.close();

  reactor->poll();

  verifyEventSequence({ChannelRegistered{}, DataAvailable{}, ChannelClosed{}, ChannelDeregistered{}});
}

TEST_F(TestFixture, ServerInitiatedCloseSequence) {
  reactor->poll();

  verifyEventSequence({ChannelRegistered{}});

  clientChannel->close();

  verifyEventSequence({ChannelRegistered{}, ChannelClosed{}, ChannelDeregistered{}});
}

TEST_F(TestFixture, ReadSequence) {
  mock.send(PAYLOAD, sizeof(PAYLOAD));

  reactor->poll();

  verifyEventSequence({ChannelRegistered{}, DataAvailable{}});
  verifyDataRead(PAYLOAD, sizeof(PAYLOAD));
}

// TODO: Test reading into invalid ByteRange
