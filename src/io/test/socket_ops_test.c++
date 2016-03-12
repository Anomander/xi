#include "xi/io/detail/socket_ops.h"
#include "tcp_socket_mock.h"

#include <gtest/gtest.h>

using namespace xi;
using namespace xi::io;

auto ALLOC = make< pooled_byte_buf_allocator >();

class client : public ::testing::Test {
protected:
  test::tcp_acceptor_mock _acceptor;
  test::tcp_socket_mock _client;
  int _descriptor;

protected:
  void SetUp() override {
    _acceptor.listen(12345);
    _client.connect(12345);
    _descriptor = _acceptor.accept();
  }

  void TearDown() override {
    _acceptor.close();
  }

  vector<u8> prepare_data(usize size) {
    auto data = vector<u8> (size);
    u16 i = 0;
    std::generate_n(begin(data), size, [&i] { return i++; });
    return move(data);
  }

  auto read(mut<byte_buf> b) {
    return io::detail::socket::read(_descriptor, b);
  }
};

TEST_F(client, read_into_empty_buf) {
  auto in = prepare_data (100);
  _client.send(in.data(), in.size());

  auto b = ALLOC->allocate(0);
  auto ret = io::detail::socket::read(_descriptor, edit(b));
  ASSERT_FALSE(ret.has_error());
  ASSERT_EQ(0, ret);
}

TEST_F(client, read_into_smaller_buf) {
  auto in = prepare_data (100);
  _client.send(in.data(), in.size());

  auto b = ALLOC->allocate(50);
  auto ret = read(edit(b));
  ASSERT_FALSE(ret.has_error());
  ASSERT_EQ(50, ret);
}

TEST_F(client, read_from_closed_conn) {
  _client.close();

  auto b = ALLOC->allocate(50);
  auto ret = read(edit(b));
  ASSERT_TRUE(ret.has_error());
  ASSERT_EQ(error::kEOF, ret.error());
  ASSERT_EQ(50U, b->writable_size());
  ASSERT_EQ(0U, b->readable_size());
}

TEST_F(client, read_some_data_and_close) {
  auto in = prepare_data (40);
  _client.send(in.data(), in.size());
  _client.close();

  auto b = ALLOC->allocate(50);
  auto ret = read(edit(b));
  ASSERT_FALSE(ret.has_error());
  ASSERT_EQ(10U, b->writable_size());
  ASSERT_EQ(40U, b->readable_size());

  ret = read(edit(b));
  ASSERT_TRUE(ret.has_error());
  ASSERT_EQ(error::kEOF, ret.error());
  ASSERT_EQ(10U, b->writable_size());
  ASSERT_EQ(40U, b->readable_size());
}

TEST_F(client, read_more_data_and_close) {
  auto in = prepare_data (100);
  _client.send(in.data(), in.size());
  _client.close();

  auto b = ALLOC->allocate(50);

  auto ret = read(edit(b));
  ASSERT_FALSE(ret.has_error());
  ASSERT_EQ(0U, b->writable_size());
  ASSERT_EQ(50U, b->readable_size());

  b = ALLOC->allocate(50);
  ret = read(edit(b));
  ASSERT_FALSE(ret.has_error());
  ASSERT_EQ(0U, b->writable_size());
  ASSERT_EQ(50U, b->readable_size());

  b = ALLOC->allocate(50);
  ret = read(edit(b));
  ASSERT_TRUE(ret.has_error());
  ASSERT_EQ(error::kEOF, ret.error());
  ASSERT_EQ(50U, b->writable_size());
  ASSERT_EQ(0U, b->readable_size());
}

// TEST_F(client, read_exactly_IOV_MAX_blocks) {
//   const auto MAX = IOV_MAX;
//   auto in = prepare_data (100 * MAX);
//   _client.send(in.data(), in.size());

//   auto b = ALLOC->allocate(1024);
//   for (auto i = 0UL; i < MAX; ++i) {
//     b->expand(100);
//   }
//   auto ret = read(edit(b));
//   ASSERT_FALSE(ret.has_error());
//   ASSERT_EQ(100 * MAX, ret);
//   ASSERT_EQ(0U, b->writable_size());
//   ASSERT_EQ(100U * MAX, b->readable_size());
// }

// TEST_F(client, read_twice_IOV_MAX_blocks) {
//   const auto MAX = IOV_MAX * 2;
//   auto in = prepare_data (100 * MAX);
//   _client.send(in.data(), in.size());

//   auto b = ALLOC->allocate(1024);
//   for (auto i = 0UL; i < MAX; ++i) {
//     b->expand(100);
//   }
//   auto ret = read(edit(b));
//   ASSERT_FALSE(ret.has_error());
//   ASSERT_EQ(100 * MAX, ret);
//   ASSERT_EQ(0U, b->writable_size());
//   ASSERT_EQ(100U * MAX, b->readable_size());
// }

// TEST_F(client, read_twice_IOV_MAX_blocks_and_close) {
//   const auto MAX = IOV_MAX * 2;
//   auto in = prepare_data (100 * MAX);
//   _client.send(in.data(), in.size());
//   _client.close();

//   auto b = ALLOC->allocate(1024);
//   for (auto i = 0UL; i < MAX; ++i) {
//     b->expand(100);
//   }

//   auto ret = read(edit(b));
//   ASSERT_FALSE(ret.has_error());
//   ASSERT_EQ(100 * MAX, ret);
//   ASSERT_EQ(0U, b->writable_size());
//   ASSERT_EQ(100U * MAX, b->readable_size());

//   b->expand(1);
//   ret = read(edit(b));
//   ASSERT_TRUE(ret.has_error());
//   ASSERT_EQ(error::kEOF, ret.error());
// }
