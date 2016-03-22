#include "xi/io/basic_buffer_allocator.h"
#include "xi/io/channel_options.h"
#include "xi/io/detail/heap_buffer_storage_allocator.h"
#include "xi/io/net/socket.h"
#include "tcp_socket_mock.h"

#include <gtest/gtest.h>

using namespace xi;
using namespace xi::io;
using namespace xi::io::net;

#define _SYSTEM_ERROR(Type, E, ...)                                            \
  try {                                                                        \
    __VA_ARGS__;                                                               \
  } catch (system_error const& e) {                                            \
    BOOST_PP_CAT(Type, _EQ)(E, e.code().value());                              \
  } catch (...) {                                                              \
    FAIL();                                                                    \
  }

#define ASSERT_SYSTEM_ERROR(E, ...) _SYSTEM_ERROR(ASSERT, E, __VA_ARGS__)
#define EXPECT_SYSTEM_ERROR(E, ...) _SYSTEM_ERROR(EXPECT, E, __VA_ARGS__)

auto ALLOC = make<
    basic_buffer_allocator< io::detail::heap_buffer_storage_allocator > >();

class stream_server_test : public ::testing::Test {
protected:
  u16 PORT = rand() % 32768 + 1024;
  unique_ptr< test::tcp_socket_mock > _client;
  unique_ptr< stream_server_socket > _server;

protected:
  void SetUp() override {
    ++PORT;
    _client = make_unique< test::tcp_socket_mock >();
    _server = make_unique< stream_server_socket >(kInet, kTCP);
    _server->set_option(option::socket::reuse_address::yes);
    endpoint< kInet > ep{PORT};
    _server->bind(ep.to_posix());
  }

  void TearDown() override {
    _client->close();
    _client.reset();
    _server.reset();
  }

  vector< u8 > prepare_data(usize size) {
    auto data = vector< u8 >(size);
    u16 i     = 0;
    std::generate_n(begin(data), size, [&i] { return i++; });
    return move(data);
  }

  auto accept() {
    return _server->accept();
  }
};

TEST_F(stream_server_test,
       accept_no_client_no_block /* read in Marley's voice */) {
  auto conn = accept();
  EXPECT_TRUE(conn.has_error());
  EXPECT_EQ(EAGAIN, conn.error().value());
}

TEST_F(stream_server_test, accept_client) {
  _client->connect(PORT);
  auto conn = accept();
  EXPECT_FALSE(conn.has_error());
}

TEST_F(stream_server_test, double_bind) {
  stream_server_socket s1(kInet, kTCP), s2(kInet, kTCP);

  endpoint< kInet > ep{19191};
  s1.bind(ep.to_posix());
  EXPECT_SYSTEM_ERROR(EINVAL, s1.bind(ep.to_posix()));
  EXPECT_SYSTEM_ERROR(EADDRINUSE, s2.bind(ep.to_posix()));

  // option doesn't change anything
  s2.set_option(option::socket::reuse_address::yes);
  EXPECT_SYSTEM_ERROR(EADDRINUSE, s2.bind(ep.to_posix()));

  s1.close();

  // succeeds now
  s2.bind(ep.to_posix());
}

TEST_F(stream_server_test, connect_and_close_before_accept) {
  _client->connect(PORT);
  _client->close();

  auto conn = accept();
  EXPECT_FALSE(conn.has_error());

  auto b   = ALLOC->allocate(50);
  auto ret = conn.value().read(edit(b));
  EXPECT_TRUE(ret.has_error());
  EXPECT_EQ(io::error::kEOF, ret.error());
}
