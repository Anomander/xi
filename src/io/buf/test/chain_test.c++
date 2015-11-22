#include "xi/io/buf/buffer.h"
#include "xi/io/buf/chain.h"
#include "xi/io/buf/iterator.h"
#include "xi/io/buf/heap_buf.h"

#include <gtest/gtest.h>

#include "src/test/util.h"

using namespace xi;
using namespace xi::io;

struct trackable_heap_buf : public heap_buf, public test::object_tracker {
  using heap_buf::heap_buf;
};

namespace {
auto build_buffer(size_t headroom, size_t size, size_t tailroom) {
  auto total_size = headroom + size + tailroom;
  auto b =
      make< buffer >(make< trackable_heap_buf >(total_size), 0, total_size);
  b->append_tail(headroom + size);
  b->consume_head(headroom);
  return move(b);
}
}

TEST(interface, empty_chain) {
  buffer::chain c;
  ASSERT_EQ(c.begin(), c.end());
  ASSERT_EQ(0UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
}

TEST(interface, range_pop_on_empty_chain_produces_empty_chain) {
  buffer::chain src;
  auto c = src.pop(begin(src), end(src));
  ASSERT_EQ(c.begin(), c.end());
  ASSERT_EQ(0UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
}

TEST(interface, push_back_empty_buffer) {
  buffer::chain c;
  auto b = make< buffer >(make< trackable_heap_buf >(1024), 0, 100);
  c.push_back(move(b));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(0UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(100UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, push_back_consumed_buffer_resets_it) {
  buffer::chain c;

  c.push_back(build_buffer(100, 0, 0));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(0UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(100UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, push_back_full_buffer) {
  buffer::chain c;
  c.push_back(build_buffer(0, 100, 0));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(100UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
  ASSERT_EQ(0UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, push_back_two_full_buffers) {
  buffer::chain c;

  c.push_back(build_buffer(0, 100, 0));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(100UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
  ASSERT_EQ(0UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  c.push_back(build_buffer(0, 100, 0));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(200UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
  ASSERT_EQ(0UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, push_back_half_full_buffer) {
  buffer::chain c;
  c.push_back(build_buffer(0, 50, 50));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(50UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(50UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, push_back_full_and_half_full_buffer) {
  buffer::chain c;

  c.push_back(build_buffer(0, 100, 0));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(100UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
  ASSERT_EQ(0UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  c.push_back(build_buffer(0, 50, 50));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(150UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(50UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, push_back_full_and_empty_buffer) {
  buffer::chain c;

  c.push_back(build_buffer(0, 100, 0));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(100UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
  ASSERT_EQ(0UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  c.push_back(build_buffer(0, 0, 100));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(100UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(100UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, push_back_half_full_and_half_full_buffer) {
  buffer::chain c;

  c.push_back(build_buffer(0, 50, 50));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(50UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(50UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  c.push_back(build_buffer(0, 50, 50));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(100UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(50UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, push_back_full_half_full_and_empty_buffer) {
  buffer::chain c;

  c.push_back(build_buffer(0, 100, 0));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(100UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
  ASSERT_EQ(0UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  c.push_back(build_buffer(0, 50, 50));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(150UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(50UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  c.push_back(build_buffer(0, 0, 100));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(150UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(150UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, push_back_half_full_at_head_buffer) {
  buffer::chain c;
  c.push_back(build_buffer(50, 50, 0));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(50UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
  ASSERT_EQ(0UL, c.tailroom());
  ASSERT_EQ(50UL, c.headroom());
}

TEST(interface, push_back_half_full_at_head_and_empty_buffer) {
  buffer::chain c;
  c.push_back(build_buffer(50, 50, 0));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(50UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
  ASSERT_EQ(0UL, c.tailroom());
  ASSERT_EQ(50UL, c.headroom());

  c.push_back(build_buffer(0, 0, 100));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(50UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(100UL, c.tailroom());
  ASSERT_EQ(50UL, c.headroom());
}

TEST(interface, push_back_two_empty_buffers) {
  buffer::chain c;

  c.push_back(build_buffer(0, 0, 100));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(0UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(100UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  c.push_back(build_buffer(0, 0, 100));
  ASSERT_NE(c.begin(), c.end());
  ASSERT_EQ(0UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(200UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, pop_buffer_on_empty_chain_produces_empty_buffer) {
  buffer::chain c;
  ASSERT_EQ(nullptr, c.pop(begin(c)));
}

TEST(interface, pop_only_buffer_in_chain) {
  buffer::chain c;
  c.push_back(build_buffer(0, 0, 100));
  ASSERT_EQ(0UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(100UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  auto b = c.pop(begin(c));
  ASSERT_EQ(0UL, b->size());
  ASSERT_EQ(100UL, b->tailroom());
  ASSERT_EQ(0UL, b->headroom());

  ASSERT_EQ(begin(c), end(c));
  ASSERT_EQ(0UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
  ASSERT_EQ(0UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, pop_the_only_tail_buffer) {
  buffer::chain c;
  c.push_back(build_buffer(0, 100, 0));
  c.push_back(build_buffer(0, 0, 100));
  ASSERT_EQ(100UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(100UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  auto b = c.pop(begin(c) + 100);
  ASSERT_EQ(0UL, b->size());
  ASSERT_EQ(100UL, b->tailroom());
  ASSERT_EQ(0UL, b->headroom());

  ASSERT_EQ(100UL, c.size());
  ASSERT_FALSE(c.has_tailroom());
  ASSERT_EQ(0UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, pop_first_tail_buffer) {
  buffer::chain c;
  c.push_back(build_buffer(0, 100, 0));
  c.push_back(build_buffer(0, 50, 50));
  c.push_back(build_buffer(0, 0, 100));
  ASSERT_EQ(150UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(150UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  auto b = c.pop(begin(c) + 100);
  ASSERT_EQ(50UL, b->size());
  ASSERT_EQ(50UL, b->tailroom());
  ASSERT_EQ(0UL, b->headroom());

  ASSERT_EQ(100UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(100UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}

TEST(interface, pop_last_tail_buffer) {
  buffer::chain c;
  c.push_back(build_buffer(0, 100, 0));
  c.push_back(build_buffer(0, 50, 50));
  c.push_back(build_buffer(0, 0, 100));
  ASSERT_EQ(150UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(150UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());

  auto b = c.pop(begin(c) + 150);
  ASSERT_EQ(0UL, b->size());
  ASSERT_EQ(100UL, b->tailroom());
  ASSERT_EQ(0UL, b->headroom());

  ASSERT_EQ(150UL, c.size());
  ASSERT_TRUE(c.has_tailroom());
  ASSERT_EQ(50UL, c.tailroom());
  ASSERT_EQ(0UL, c.headroom());
}
