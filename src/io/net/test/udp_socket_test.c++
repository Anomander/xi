#include "xi/io/basic_buffer_allocator.h"
#include "xi/io/net/socket2.h"
#include "src/test/base.h"
#include "udp_socket_mock.h"

#include <gtest/gtest.h>

using namespace xi;
using namespace xi::io;
using namespace xi::io::net;

auto ALLOC =
    make< basic_buffer_allocator >(make< exact_fragment_allocator >());

namespace xi {
namespace test {

  class client : public ::testing::Test {
  protected:
    io::udp_socket_mock _remote;
    unique_ptr< datagram_socket > _socket;
    endpoint< kInet > _last_endpoint;

  protected:
    void SetUp() override {
      _remote.bind(54321);
      _remote.connect(12345);
      _socket = make_unique< datagram_socket >(kInet, kUDP);
      endpoint< kInet > ep{12345};
      _socket->bind(ep.to_posix());
    }

    void TearDown() override {
      _remote.close();
    }

    vector< u8 > prepare_data(usize size) {
      auto data = vector< u8 >(size);
      u16 i     = 0;
      std::generate_n(begin(data), size, [&i] { return i++; });
      return data;
    }

    auto read(mut< buffer > b) {
      return _socket->read_buffer_from(b, _last_endpoint.to_posix());
    }

    auto write(mut< buffer > b) {
      return _socket->write_buffer_to(b, _last_endpoint.to_posix());
    }

    void test_big_writes(int);
  };

  TEST_F(client, read_into_empty_buf) {
    auto in = prepare_data(100);
    _remote.send(in.data(), in.size());

    auto b   = ALLOC->allocate(0, 10, 0);
    auto ret = read(edit(b));
    EXPECT_FALSE(ret.has_error());
    EXPECT_EQ(0, ret);
  }

  TEST_F(client, read_into_smaller_buf) {
    auto in = prepare_data(100);
    _remote.send(in.data(), in.size());

    auto b   = ALLOC->allocate(50);
    auto ret = read(edit(b));
    EXPECT_FALSE(ret.has_error());
    EXPECT_EQ(50, ret);
    EXPECT_EQ(0U, b->tailroom());
    EXPECT_EQ(50U, b->size());
  }

  TEST_F(client, read_from_closed_conn) {
    _remote.close();

    auto b   = ALLOC->allocate(50);
    auto ret = read(edit(b));
    EXPECT_TRUE(ret.has_error());
    EXPECT_EQ(error::kRetry, ret.error());
    EXPECT_EQ(50U, b->tailroom());
    EXPECT_EQ(0U, b->size());
  }

  TEST_F(client, read_some_data_and_close) {
    auto in = prepare_data(40);
    _remote.send(in.data(), in.size());
    _remote.close();

    auto b   = ALLOC->allocate(50);
    auto ret = read(edit(b));
    EXPECT_FALSE(ret.has_error());
    EXPECT_EQ(10U, b->tailroom());
    EXPECT_EQ(40U, b->size());

    ret = read(edit(b));
    EXPECT_TRUE(ret.has_error());
    EXPECT_EQ(error::kRetry, ret.error());
    EXPECT_EQ(10U, b->tailroom());
    EXPECT_EQ(40U, b->size());
  }

  TEST_F(client, read_truncated_data_and_close) {
    auto in = prepare_data(100);
    _remote.send(in.data(), in.size());
    _remote.close();

    auto b = ALLOC->allocate(50);

    auto ret = read(edit(b));
    EXPECT_FALSE(ret.has_error());
    EXPECT_EQ(0U, b->tailroom());
    EXPECT_EQ(50U, b->size());

    b   = ALLOC->allocate(50);
    ret = read(edit(b));
    EXPECT_TRUE(ret.has_error());
    EXPECT_EQ(error::kRetry, ret.error());
  }

  TEST_F(client, large_messages_are_rejected) {
    auto b  = ALLOC->allocate(100 * 1024);
    auto in = prepare_data(100 * 1024);
    b->write(byte_range{in});
    EXPECT_EQ(100u * 1024, b->size());
    EXPECT_EQ(1u, b->length());

    auto ret = write(edit(b));
    EXPECT_TRUE(ret.has_error());
    EXPECT_EQ(error_from_value(EMSGSIZE), ret.error());
  }
}
}
