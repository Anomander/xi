#include "xi/io/basic_buffer_allocator.h"
#include "xi/io/channel_options.h"
#include "xi/io/detail/heap_buffer_storage_allocator.h"
#include "xi/io/net/socket.h"
#include "tcp_socket_mock.h"

#include <gtest/gtest.h>

using namespace xi;
using namespace xi::io;
using namespace xi::io::net;

auto ALLOC = make<
    basic_buffer_allocator< io::detail::heap_buffer_storage_allocator > >();

class client : public ::testing::Test {
protected:
  test::tcp_acceptor_mock _acceptor;
  test::tcp_socket_mock _remote;
  unique_ptr< stream_client_socket > _socket;

protected:
  void SetUp() override {
    _acceptor.bind(12345);
    _remote.connect(12345);
    _socket = make_unique< stream_client_socket >(_acceptor.accept());
  }

  void TearDown() override {
    _acceptor.close();
  }

  vector< u8 > prepare_data(usize size) {
    auto data = vector< u8 >(size);
    u16 i     = 0;
    std::generate_n(begin(data), size, [&i] { return i++; });
    return move(data);
  }

  auto read(mut< buffer > b) {
    return _socket->read(b);
  }

  auto write(mut< buffer > b) {
    return _socket->write(b);
  }

  void test_big_writes(int);
};

TEST_F(client, read_into_empty_buf) {
  auto in = prepare_data(100);
  _remote.send(in.data(), in.size());

  auto b   = ALLOC->allocate(0, 10, 0);
  auto ret = read(edit(b));
  ASSERT_FALSE(ret.has_error());
  ASSERT_EQ(0, ret);
}

TEST_F(client, read_into_smaller_buf) {
  auto in = prepare_data(100);
  _remote.send(in.data(), in.size());

  auto b   = ALLOC->allocate(50);
  auto ret = read(edit(b));
  ASSERT_FALSE(ret.has_error());
  ASSERT_EQ(50, ret);
  ASSERT_EQ(0U, b->tailroom());
  ASSERT_EQ(50U, b->size());
}

TEST_F(client, read_from_closed_conn) {
  _remote.close();

  auto b   = ALLOC->allocate(50);
  auto ret = read(edit(b));
  ASSERT_TRUE(ret.has_error());
  ASSERT_EQ(error::kEOF, ret.error());
  ASSERT_EQ(50U, b->tailroom());
  ASSERT_EQ(0U, b->size());
}

TEST_F(client, read_some_data_and_close) {
  auto in = prepare_data(40);
  _remote.send(in.data(), in.size());
  _remote.close();

  auto b   = ALLOC->allocate(50);
  auto ret = read(edit(b));
  ASSERT_FALSE(ret.has_error());
  ASSERT_EQ(10U, b->tailroom());
  ASSERT_EQ(40U, b->size());

  ret = read(edit(b));
  ASSERT_TRUE(ret.has_error());
  ASSERT_EQ(error::kEOF, ret.error());
  ASSERT_EQ(10U, b->tailroom());
  ASSERT_EQ(40U, b->size());
}

TEST_F(client, read_more_data_and_close) {
  auto in = prepare_data(100);
  _remote.send(in.data(), in.size());
  _remote.close();

  auto b = ALLOC->allocate(50);

  auto ret = read(edit(b));
  ASSERT_FALSE(ret.has_error());
  ASSERT_EQ(0U, b->tailroom());
  ASSERT_EQ(50U, b->size());

  b   = ALLOC->allocate(50);
  ret = read(edit(b));
  ASSERT_FALSE(ret.has_error());
  ASSERT_EQ(0U, b->tailroom());
  ASSERT_EQ(50U, b->size());

  b   = ALLOC->allocate(50);
  ret = read(edit(b));
  ASSERT_TRUE(ret.has_error());
  ASSERT_EQ(error::kEOF, ret.error());
  ASSERT_EQ(50U, b->tailroom());
  ASSERT_EQ(0U, b->size());
}

void
client::test_big_writes(int how_big) {
  auto b = ALLOC->allocate(100 * how_big);
  auto in  = prepare_data(100 * how_big);
  b->write(byte_range{in});
  EXPECT_EQ(100u * how_big, b->size());
  EXPECT_EQ(1u, b->length());

  auto ret = write(edit(b));
  ASSERT_FALSE(ret.has_error());
  EXPECT_EQ(100 * how_big, ret);
  EXPECT_EQ(0U, b->tailroom());
  EXPECT_EQ(0U, b->size());

  vector< u8 > out(100 * how_big);
  ::bzero(out.data(), out.size());
  usize wret = _remote.recv(out.data(), out.size());
  if (wret < out.size()) {
    wret += _remote.recv(out.data() + wret, out.size() - wret);
  }
  EXPECT_EQ(100 * how_big, ret);
  EXPECT_EQ(in, out);
}

TEST_F(client, write_exactly_IOV_MAX_blocks) {
  test_big_writes(IOV_MAX);
}

TEST_F(client, write_IOV_MAX_blocks_plus_one) {
  test_big_writes(IOV_MAX + 1);
}

TEST_F(client, write_twice_IOV_MAX_blocks) {
  test_big_writes(2 * IOV_MAX);
}
