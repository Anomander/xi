#include "io/AsyncChannel.h"
#include "io/DataMessage.h"
#include "async/libevent/Executor.h"
#include "io/test/TcpSocketMock.h"

#include <gtest/gtest.h>

#include <boost/thread.hpp>

using namespace xi;
using namespace xi::io;
using namespace xi::async::libevent;

// void * operator new(size_t count) {
//   auto * ret = ::malloc (count);
//   std::cout << "Newd " << ret << std::endl;
//   return ret;
// }

// void operator delete (void * p) {
//   std::cout << "deleted " << p << std::endl;
//   ::free (p);
// }

class MessageHandler
  : public pipeline::SimpleInboundPipelineFastCastHandler <DataMessage>
{
public:
  // void channelOpened (mut<pipeline::HandlerContext> cx) override {
  //   opened = true;
  // }
  // void channelClosed (mut<pipeline::HandlerContext> cx) override {
  //   opened = false;
  // }
  void channelRegistered (mut<pipeline::HandlerContext> cx) override {
    registered = true;
  }
  void channelDeregistered (mut<pipeline::HandlerContext> cx) override {
    registered = false;
  }
  void messageReceived(mut<pipeline::HandlerContext> cx, own <DataMessage> msg) override {
    ++messagesReceived;
    lastMessage = move(msg);
  }
  void channelError (mut<pipeline::HandlerContext> cx, error_code error) override {
    lastError = error;
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
    executor = make <Executor>();
    handler = make <MessageHandler>();
    mock = make <test::TcpSocketMock> ();
    auto ch = make <ServerChannel<kInet, kTCP>>();
    ch->setOption (ReuseAddress::yes);
    ch->bind(12345);
    ch->childHandler ([&](auto ch) {
        ch->pipeline()->pushBack(handler);
        executor->attachHandler(move(ch));
      });
    executor->attachHandler (move(ch));

    mock.connect (12345);
    executor->runOnce();
  }

protected:
  own<Executor> executor;
  own<MessageHandler> handler;
  own<test::TcpSocketMock> mock;
};

TEST_F (TestFixture, RegisterOnConnect) {
  executor->runOnce();

  ASSERT_EQ(true, handler->registered);
}

TEST_F (TestFixture, Read) {
  mock.sendMessage (PAYLOAD, sizeof(PAYLOAD));

  executor->runOnce();

  ASSERT_EQ (1UL, handler->messagesReceived);
  ASSERT_EQ (sizeof(PAYLOAD), handler->lastMessage->data()->header().size);
  ASSERT_EQ (0, memcmp(PAYLOAD, handler->lastMessage->data()->readableRange().data, sizeof(PAYLOAD)));
}

TEST_F (TestFixture, ReadNoPayload) {
  mock.sendMessage (nullptr, 0);

  executor->runOnce();

  ASSERT_EQ (1UL, handler->messagesReceived);
  ASSERT_EQ (0UL, handler->lastMessage->data()->header().size);
}

TEST_F (TestFixture, RemoteCloseAfterHeader) {
  ProtocolHeader hdr;
  hdr.size = 100;
  mock.send(&hdr, sizeof(hdr));

  executor->runOnce();

  ASSERT_EQ (0UL, handler->messagesReceived);

  mock.close();

  executor->runOnce();

  ASSERT_EQ (0UL, handler->messagesReceived);
  ASSERT_EQ (false, handler->registered);
}

// Since header is not actually read before full
// size of it is available, the connection will
// not read EOF.
// These connections will be left for the reaper
// to close.
TEST_F (TestFixture, RemoteCloseMidHeader) {
  ProtocolHeader hdr;
  mock.send(&hdr, sizeof(hdr)/ 2);

  executor->runOnce();

  ASSERT_EQ (0UL, handler->messagesReceived);

  mock.close();

  executor->runOnce();

  ASSERT_EQ (0UL, handler->messagesReceived);
  ASSERT_EQ (true, handler->registered);
}

TEST_F (TestFixture, RemoteCloseAfterPayloadSameEvent) {
  mock.sendMessage (PAYLOAD, sizeof(PAYLOAD));
  mock.close();

  executor->runOnce();

  ASSERT_EQ (1UL, handler->messagesReceived);
  ASSERT_EQ (false, handler->registered);
  ASSERT_EQ (sizeof(PAYLOAD), handler->lastMessage->data()->header().size);
  ASSERT_EQ (0, memcmp(PAYLOAD, handler->lastMessage->data()->readableRange().data, sizeof(PAYLOAD)));
}

TEST_F (TestFixture, RemoteCloseAfterPayload) {
  mock.sendMessage (PAYLOAD, sizeof(PAYLOAD));

  executor->runOnce();

  ASSERT_EQ (1UL, handler->messagesReceived);

  mock.close();

  executor->runOnce();

  ASSERT_EQ (1UL, handler->messagesReceived);
  ASSERT_EQ (false, handler->registered);
  ASSERT_EQ (sizeof(PAYLOAD), handler->lastMessage->data()->header().size);
  ASSERT_EQ (0, memcmp(PAYLOAD, handler->lastMessage->data()->readableRange().data, sizeof(PAYLOAD)));
}

TEST_F (TestFixture, RemoteCloseMidPayload) {
  ProtocolHeader hdr;
  hdr.size = sizeof(PAYLOAD) * 2;
  mock.send(&hdr, sizeof(hdr));
  mock.send(PAYLOAD, sizeof(PAYLOAD));

  executor->runOnce();

  ASSERT_EQ (0UL, handler->messagesReceived);

  mock.close();

  executor->runOnce();

  ASSERT_EQ (0UL, handler->messagesReceived);
  ASSERT_EQ (false, handler->registered);
}

TEST_F (TestFixture, ReadPayloadSeveralEvents) {
  ProtocolHeader hdr;
  hdr.size = sizeof(PAYLOAD);
  mock.send(&hdr, sizeof(hdr));
  /// size better be even
  mock.send(PAYLOAD, sizeof(PAYLOAD) / 2);

  executor->runOnce();

  ASSERT_EQ (0UL, handler->messagesReceived);

  mock.send(PAYLOAD + sizeof(PAYLOAD)/2, sizeof(PAYLOAD) / 2);

  executor->runOnce();

  ASSERT_EQ (1UL, handler->messagesReceived);
  ASSERT_EQ (sizeof(PAYLOAD), handler->lastMessage->data()->header().size);
  ASSERT_EQ (0, memcmp(PAYLOAD, handler->lastMessage->data()->readableRange().data, sizeof(PAYLOAD)));
}
