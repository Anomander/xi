#include "xi/io/async_channel.h"
#include "xi/io/data_message.h"
#include "xi/async/libevent/reactor.h"
#include "xi/io/pipeline/pipeline_handler.h"
#include "xi/io/pipeline/util.h"
#include "src/test/test_kernel.h"
#include "xi/core/kernel_utils.h"

#include "tcp_socket_mock.h"

#include <gtest/gtest.h>

using namespace xi;
using namespace xi::io;
using namespace xi::io::pipeline;
using namespace xi::async::libevent;

using xi::test::test_kernel;
using xi::test::kCurrentThread;
using xi::io::test::tcp_socket_mock;

namespace xi {
namespace io {
  namespace pipeline {
    bool operator==(event const &lhs, event const &rhs) {
      return typeid(lhs) == typeid(rhs);
    }
    bool operator==(channel_error const &lhs, channel_error const &rhs) {
      return lhs.error() == rhs.error();
    }
    std::ostream &operator<<(std::ostream &os, channel_error const &rhs) {
      return os << typeid(rhs).name();
    }
    std::ostream &operator<<(std::ostream &os, channel_exception const &rhs) {
      return os << typeid(rhs).name();
    }
    std::ostream &operator<<(std::ostream &os, channel_registered const &rhs) {
      return os << typeid(rhs).name();
    }
    std::ostream &operator<<(std::ostream &os,
                             channel_deregistered const &rhs) {
      return os << typeid(rhs).name();
    }
    std::ostream &operator<<(std::ostream &os, channel_closed const &rhs) {
      return os << typeid(rhs).name();
    }
    std::ostream &operator<<(std::ostream &os, data_available const &rhs) {
      return os << typeid(rhs).name();
    }
    std::ostream &operator<<(std::ostream &os, event const &rhs) {
      return os << typeid(rhs).name();
    }
  }
}
}

using any_event =
    variant< channel_closed, channel_registered, channel_deregistered,
             data_available, channel_error, channel_exception >;

class message_handler : public xi::io::pipeline::pipeline_handler {
public:
  void handle_event(mut< xi::io::pipeline::handler_context > cx,
                    xi::io::pipeline::channel_closed e) override {
    events.emplace_back(e);
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    cx->forward(e);
  }
  void handle_event(mut< xi::io::pipeline::handler_context > cx,
                    xi::io::pipeline::channel_registered e) override {
    events.emplace_back(e);
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    cx->forward(e);
  }
  void handle_event(mut< xi::io::pipeline::handler_context > cx,
                    xi::io::pipeline::channel_deregistered e) override {
    events.emplace_back(e);
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    cx->forward(e);
  }
  void handle_event(mut< xi::io::pipeline::handler_context > cx,
                    xi::io::pipeline::data_available e) override {
    events.emplace_back(e);
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    size_read = cx->get_channel()->read(byte_range{data, sizeof(data)});
    cx->forward(e);
  }
  void handle_event(mut< xi::io::pipeline::handler_context > cx,
                    xi::io::pipeline::channel_error e) override {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    events.emplace_back(e);
    cx->forward(e);
  }
  void handle_event(mut< xi::io::pipeline::handler_context > cx,
                    xi::io::pipeline::channel_exception e) override {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    events.emplace_back(e);
    cx->forward(e);
  }

public:
  vector< any_event > events;
  uint8_t data[1024];
  size_t size_read = 0;
};

uint8_t PAYLOAD[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

class test_fixture : public ::testing::Test {
protected:
  void SetUp() override {
    kernel = make< test_kernel >();
    pool = make_executor_pool(edit(kernel));
    _reactor = make< reactor >();
    _reactor->attach_executor(pool->share_executor(kCurrentThread));
    handler = make< message_handler >();
    mock = make< tcp_socket_mock >();
    auto ch = make< server_channel< kInet, kTCP > >();
    ch->set_option(reuse_address::yes);
    ch->bind(12345);
    ch->child_handler([&](auto channel) {
      channel->get_pipeline()->push_back(handler);
      client_channel = edit(channel);
      _reactor->attach_handler(move(channel));
    });
    _reactor->attach_handler(move(ch));

    mock.connect(12345);
    _reactor->poll();
  }

  void tear_down() {
    kernel->run_core(kCurrentThread);
    release(move(kernel));
    release(move(pool));
    release(move(_reactor));
    release(move(handler));
  }

  void verify_event_sequence(initializer_list< any_event > events) {
    ASSERT_EQ(events.size(), handler->events.size());
    size_t i = 0;
    for (auto &&e : events) { ASSERT_EQ(e, handler->events[i++]); }
  }

  void verify_data_read(uint8_t *data, size_t size) {
    ASSERT_EQ(size, handler->size_read);
    for (size_t i = 0; i < size; ++i) { ASSERT_EQ(data[i], handler->data[i]); }
  }

protected:
  own< test_kernel > kernel;
  own< core::executor_pool > pool;
  own< reactor > _reactor;
  own< message_handler > handler;
  own< tcp_socket_mock > mock;
  mut< async_channel > client_channel;
};

TEST_F(test_fixture, client_initiated_connect_sequence) {
  _reactor->poll();

  verify_event_sequence({channel_registered{}});
}

TEST_F(test_fixture, client_initiated_close_sequence) {
  _reactor->poll();

  verify_event_sequence({channel_registered{}});

  mock.close();

  _reactor->poll();

  verify_event_sequence(
      {channel_registered{}, data_available{}, channel_closed{}});

  kernel->run_core(kCurrentThread);

  verify_event_sequence({channel_registered{}, data_available{},
                         channel_closed{}, channel_deregistered{}});
}

TEST_F(test_fixture, server_initiated_close_sequence) {
  _reactor->poll();

  verify_event_sequence({channel_registered{}});

  client_channel->close();

  verify_event_sequence({channel_registered{}, channel_closed{}});

  kernel->run_core(kCurrentThread);

  verify_event_sequence(
      {channel_registered{}, channel_closed{}, channel_deregistered{}});
}

TEST_F(test_fixture, read_sequence) {
  mock.send(PAYLOAD, sizeof(PAYLOAD));

  _reactor->poll();

  verify_event_sequence({channel_registered{}, data_available{}});
  verify_data_read(PAYLOAD, sizeof(PAYLOAD));
}

// TODO: test reading into invalid byte_range
