#include "io/AsyncChannel.h"
#include "io/DataMessage.h"
#include "async/libevent/Reactor.h"
#include "io/test/TcpSocketMock.h"
#include "io/pipeline/Util.h"

#include <gtest/gtest.h>

#include <boost/thread.hpp>

using namespace xi;
using namespace xi::io;
using namespace xi::async::libevent;

class MessageHandler
  : public pipeline::SimpleInboundPipelineHandler <DataMessage>
{
public:
  // void channelOpened (mut<pipeline::HandlerContext> cx) override {
  //   opened = true;
  // }
  // void channelClosed (mut<pipeline::HandlerContext> cx) override {
  //   opened = false;
  // }
  void handleEvent (mut<pipeline::HandlerContext> cx, pipeline::ChannelRegistered) override {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    registered = true;
  }
  void handleEvent (mut<pipeline::HandlerContext> cx, pipeline::ChannelDeregistered) override {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    registered = false;
  }
  void messageReceived(mut<pipeline::HandlerContext> cx, own <DataMessage> msg) override {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    ++messagesReceived;
    lastMessage = move(msg);
  }
  void handleEvent (mut<pipeline::HandlerContext> cx, pipeline::ChannelError error) override {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    lastError = error.error();
  }

public:
  size_t messagesReceived = 0UL;
  bool registered = false;
  bool opened = false;
  error_code lastError;
  own<DataMessage> lastMessage;
};

uint8_t PAYLOAD [10] = {0,1,2,3,4,5,6,7,8,9};

class TestFixture : public ::testing::Test {
protected:
  void SetUp() override {
    reactor = make <Reactor>();
    handler = make <MessageHandler>();
    mock = make <test::TcpSocketMock> ();
    auto ch = make <ServerChannel<kInet, kTCP>>();
    ch->setOption (ReuseAddress::yes);
    ch->bind(12345);
    ch->pipeline()->pushBack(
      pipeline::makeInboundHandler <ClientChannelConnected> (
        [&] (auto cx, auto msg) {
          msg->channel()->pipeline()->pushBack(handler);
          reactor->attachHandler(msg->extractChannel());
        }
      )
    );
    reactor->attachHandler (move(ch));

    mock.connect (12345);
    reactor->poll();
  }

protected:
  own<Reactor> reactor;
  own<MessageHandler> handler;
  own<test::TcpSocketMock> mock;
};

TEST_F (TestFixture, RegisterOnConnect) {
  reactor->poll();

  ASSERT_EQ(true, handler->registered);
}

TEST_F (TestFixture, Read) {
  mock.sendMessage (PAYLOAD, sizeof(PAYLOAD));

  reactor->poll();

  ASSERT_EQ (1UL, handler->messagesReceived);
  ASSERT_EQ (sizeof(PAYLOAD), handler->lastMessage->data()->header().size);
  ASSERT_EQ (0, memcmp(PAYLOAD, handler->lastMessage->data()->readableRange().data, sizeof(PAYLOAD)));
}

TEST_F (TestFixture, ReadNoPayload) {
  mock.sendMessage (nullptr, 0);

  reactor->poll();

  ASSERT_EQ (1UL, handler->messagesReceived);
  ASSERT_EQ (0UL, handler->lastMessage->data()->header().size);
}

TEST_F (TestFixture, RemoteCloseAfterHeader) {
  ProtocolHeader hdr;
  hdr.size = 100;
  mock.send(&hdr, sizeof(hdr));

  reactor->poll();

  ASSERT_EQ (0UL, handler->messagesReceived);

  mock.close();

  reactor->poll();

  ASSERT_EQ (0UL, handler->messagesReceived);
  ASSERT_EQ (handler->registered, false);
}

// Since header is not actually read before full
// size of it is available, the connection will
// not read EOF.
// These connections will be left for the reaper
// to close.
TEST_F (TestFixture, RemoteCloseMidHeader) {
  ProtocolHeader hdr;
  mock.send(&hdr, sizeof(hdr)/ 2);

  reactor->poll();

  ASSERT_EQ (0UL, handler->messagesReceived);

  mock.close();

  reactor->poll();

  ASSERT_EQ (0UL, handler->messagesReceived);
  ASSERT_EQ (handler->registered, true);
}

TEST_F (TestFixture, RemoteCloseAfterPayloadSameEvent) {
  mock.sendMessage (PAYLOAD, sizeof(PAYLOAD));
  mock.close();

  reactor->poll();

  ASSERT_EQ (1UL, handler->messagesReceived);
  ASSERT_EQ (handler->registered, false);
  ASSERT_EQ (sizeof(PAYLOAD), handler->lastMessage->data()->header().size);
  ASSERT_EQ (0, memcmp(PAYLOAD, handler->lastMessage->data()->readableRange().data, sizeof(PAYLOAD)));
}

TEST_F (TestFixture, RemoteCloseAfterPayload) {
  mock.sendMessage (PAYLOAD, sizeof(PAYLOAD));

  reactor->poll();

  ASSERT_EQ (1UL, handler->messagesReceived);

  mock.close();

  reactor->poll();

  ASSERT_EQ (1UL, handler->messagesReceived);
  ASSERT_EQ (handler->registered, false);
  ASSERT_EQ (sizeof(PAYLOAD), handler->lastMessage->data()->header().size);
  ASSERT_EQ (0, memcmp(PAYLOAD, handler->lastMessage->data()->readableRange().data, sizeof(PAYLOAD)));
}

TEST_F (TestFixture, RemoteCloseMidPayload) {
  ProtocolHeader hdr;
  hdr.size = sizeof(PAYLOAD) * 2;
  mock.send(&hdr, sizeof(hdr));
  mock.send(PAYLOAD, sizeof(PAYLOAD));

  reactor->poll();

  ASSERT_EQ (0UL, handler->messagesReceived);

  mock.close();

  reactor->poll();

  ASSERT_EQ (0UL, handler->messagesReceived);
  ASSERT_EQ (handler->registered, false);
}

TEST_F (TestFixture, ReadPayloadSeveralEvents) {
  ProtocolHeader hdr;
  hdr.size = sizeof(PAYLOAD);
  mock.send(&hdr, sizeof(hdr));
  /// size better be even
  mock.send(PAYLOAD, sizeof(PAYLOAD) / 2);

  reactor->poll();

  ASSERT_EQ (0UL, handler->messagesReceived);

  mock.send(PAYLOAD + sizeof(PAYLOAD)/2, sizeof(PAYLOAD) / 2);

  reactor->poll();

  ASSERT_EQ (1UL, handler->messagesReceived);
  ASSERT_EQ (sizeof(PAYLOAD), handler->lastMessage->data()->header().size);
  ASSERT_EQ (0, memcmp(PAYLOAD, handler->lastMessage->data()->readableRange().data, sizeof(PAYLOAD)));
}
