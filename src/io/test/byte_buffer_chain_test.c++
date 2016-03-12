#include "xi/io/byte_buffer.h"
#include "xi/io/byte_buffer_chain.h"
#include "xi/io/basic_byte_buffer_allocator.h"
#include "xi/io/detail/heap_byte_buffer_storage_allocator.h"

#include <gtest/gtest.h>

#include "src/test/util.h"

using namespace xi;
using namespace xi::io;
using namespace xi::io::detail;

auto ALLOC =
    make< basic_byte_buffer_allocator< heap_byte_buffer_storage_allocator > >();

auto make_buffer(usize total, usize written) {
  vector< u8 > in(written);
  usize i = 0u;
  std::generate_n(begin(in), written, [&] { return ++i; });

  auto b = ALLOC->allocate(total);
  b->write(byte_range{in});
  return move(b);
}

TEST(interface, empty_chain) {
  byte_buffer::chain c;
  ASSERT_TRUE(c.empty());
  ASSERT_EQ(0u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(0u, c.compute_writable_size());
}

TEST(interface, pop_on_empty_chain_produces_null) {
  byte_buffer::chain c;
  ASSERT_EQ(nullptr, c.pop_front());
  ASSERT_EQ(nullptr, c.pop_back());
}

TEST(interface, push_back_changes_stats) {
  byte_buffer::chain c;

  c.push_back(make_buffer(50, 0));

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(1u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(50u, c.compute_writable_size());

  c.push_back(make_buffer(50, 10));

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(2u, c.length());
  ASSERT_EQ(10u, c.size());
  ASSERT_EQ(40u, c.compute_writable_size());

  c.push_back(make_buffer(50, 20));

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(3u, c.length());
  ASSERT_EQ(30u, c.size());
  ASSERT_EQ(30u, c.compute_writable_size());
}

TEST(interface, pop_back_changes_stats) {
  byte_buffer::chain c;

  c.push_back(make_buffer(50, 0));
  c.push_back(make_buffer(50, 10));
  c.push_back(make_buffer(50, 20));

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(3u, c.length());
  ASSERT_EQ(30u, c.size());
  ASSERT_EQ(30u, c.compute_writable_size());

  c.pop_back();

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(2u, c.length());
  ASSERT_EQ(10u, c.size());
  ASSERT_EQ(40u, c.compute_writable_size());

  c.pop_back();

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(1u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(50u, c.compute_writable_size());

  c.pop_back();

  ASSERT_TRUE(c.empty());
  ASSERT_EQ(0u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(0u, c.compute_writable_size());
}

TEST(interface, push_back_empty_buffer_ignored) {
  byte_buffer::chain c;

  c.push_back(make_buffer(0, 0));

  ASSERT_TRUE(c.empty());
  ASSERT_EQ(0u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(0u, c.compute_writable_size());
}

TEST(interface, pop_front_the_only_buffer) {
  byte_buffer::chain c;

  c.push_back(make_buffer(50, 0));

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(1u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(50u, c.compute_writable_size());

  auto b = c.pop_front();

  ASSERT_TRUE(c.empty());
  ASSERT_EQ(0u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(0u, c.compute_writable_size());

  ASSERT_EQ(0u, b->size());
  ASSERT_EQ(50u, b->tailroom());
}

TEST(interface, pop_front_not_the_only_buffer) {
  byte_buffer::chain c;

  c.push_back(make_buffer(50, 0));
  c.push_back(make_buffer(50, 0));

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(2u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(100u, c.compute_writable_size());

  auto b = c.pop_front();

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(1u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(50u, c.compute_writable_size());

  ASSERT_EQ(0u, b->size());
  ASSERT_EQ(50u, b->tailroom());
}

TEST(interface, pop_back_the_only_buffer) {
  byte_buffer::chain c;

  c.push_back(ALLOC->allocate(50));

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(1u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(50u, c.compute_writable_size());

  auto b = c.pop_back();

  ASSERT_TRUE(c.empty());
  ASSERT_EQ(0u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(0u, c.compute_writable_size());

  ASSERT_EQ(0u, b->size());
  ASSERT_EQ(50u, b->tailroom());
}

TEST(interface, pop_back_not_the_only_buffer) {
  byte_buffer::chain c;

  c.push_back(make_buffer(50, 0));
  c.push_back(make_buffer(50, 0));

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(2u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(100u, c.compute_writable_size());

  auto b = c.pop_back();

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(1u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(50u, c.compute_writable_size());

  ASSERT_EQ(0u, b->size());
  ASSERT_EQ(50u, b->tailroom());
}

TEST(interface, push_back_pop_front_the_only_buffer) {
  byte_buffer::chain c;

  c.push_back(make_buffer(50, 0));

  ASSERT_FALSE(c.empty());
  ASSERT_EQ(1u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(50u, c.compute_writable_size());

  auto b = c.pop_front();

  ASSERT_TRUE(c.empty());
  ASSERT_EQ(0u, c.length());
  ASSERT_EQ(0u, c.size());
  ASSERT_EQ(0u, c.compute_writable_size());

  ASSERT_EQ(0u, b->size());
  ASSERT_EQ(50u, b->tailroom());
}

TEST(interface, coalesce_empty_chain) {
  byte_buffer::chain c;

  EXPECT_EQ(0u, c.coalesce(edit(ALLOC)));

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0u, c.length());
  EXPECT_EQ(0u, c.size());
}

TEST(interface, coalesce_capped_empty_chain) {
  byte_buffer::chain c;

  EXPECT_EQ(0u, c.coalesce(edit(ALLOC), 1));

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0u, c.length());
  EXPECT_EQ(0u, c.size());
}

TEST(interface, coalesce_single_empty_buffer) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 0));

  EXPECT_EQ(0u, c.coalesce(edit(ALLOC)));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(0u, c.size());
}

TEST(interface, coalesce_capped_single_empty_buffer) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 0));

  EXPECT_EQ(0u, c.coalesce(edit(ALLOC), 25));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(0u, c.size());
}

TEST(interface, coalesce_single_halffull_buffer) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 25));

  EXPECT_EQ(25u, c.coalesce(edit(ALLOC)));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(25u, c.size());
}

TEST(interface, coalesce_capped_single_halffull_buffer) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 25));

  EXPECT_EQ(25u, c.coalesce(edit(ALLOC), 10));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(25u, c.size());

  EXPECT_EQ(25u, c.coalesce(edit(ALLOC), 25));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(25u, c.size());
}

TEST(interface, coalesce_single_full_buffer) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 50));

  EXPECT_EQ(50u, c.coalesce(edit(ALLOC)));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(50u, c.size());
}

TEST(interface, coalesce_capped_single_full_buffer) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 50));

  EXPECT_EQ(50u, c.coalesce(edit(ALLOC), 10));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(50u, c.size());

  EXPECT_EQ(50u, c.coalesce(edit(ALLOC), 25));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(50u, c.size());

  EXPECT_EQ(50u, c.coalesce(edit(ALLOC), 50));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(50u, c.size());
}

TEST(interface, coalesce_multiple_enough_storage) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 10));
  c.push_back(make_buffer(50, 10));
  c.push_back(make_buffer(50, 10));
  c.push_back(make_buffer(50, 10));
  c.push_back(make_buffer(50, 10));

  auto old_head = c.head();
  EXPECT_EQ(5u, c.length());

  EXPECT_EQ(50u, c.coalesce(edit(ALLOC)));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(50u, c.size());
  EXPECT_EQ(old_head, c.head());
}

TEST(interface, coalesce_capped_multiple_enough_storage) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 10));
  c.push_back(make_buffer(50, 10));
  c.push_back(make_buffer(50, 10));
  c.push_back(make_buffer(50, 10));
  c.push_back(make_buffer(50, 10));

  auto old_head = c.head();
  EXPECT_EQ(5u, c.length());

  EXPECT_EQ(25u, c.coalesce(edit(ALLOC), 25));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(4u, c.length());
  EXPECT_EQ(50u, c.size());
  EXPECT_EQ(old_head, c.head());
}

TEST(interface, coalesce_multiple_not_enough_storage) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));

  auto old_head = c.head();
  EXPECT_EQ(5u, c.length());

  EXPECT_EQ(100u, c.coalesce(edit(ALLOC)));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(100u, c.size());
  EXPECT_NE(old_head, c.head());
}

TEST(interface, coalesce_capped_multiple_not_enough_storage) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));

  auto old_head = c.head();
  EXPECT_EQ(5u, c.length());

  EXPECT_EQ(50u, c.coalesce(edit(ALLOC), 50));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(4u, c.length());
  EXPECT_EQ(100u, c.size());
  EXPECT_EQ(50u, c.head()->size());
  EXPECT_EQ(old_head, c.head());
}

TEST(interface, split_empty_chain) {
  byte_buffer::chain c;

  auto d = c.split(1);

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0u, c.length());
  EXPECT_EQ(0u, c.size());

  EXPECT_TRUE(d.empty());
  EXPECT_EQ(0u, d.length());
  EXPECT_EQ(0u, d.size());
}

TEST(interface, split_single_empty_buffer) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 0));

  auto d = c.split(1);

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0u, c.length());
  EXPECT_EQ(0u, c.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(0u, d.size());
}

TEST(interface, split_multiple_empty_buffers) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 0));
  c.push_back(make_buffer(50, 0));
  c.push_back(make_buffer(50, 0));

  auto d = c.split(1);

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0u, c.length());
  EXPECT_EQ(0u, c.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(3u, d.length());
  EXPECT_EQ(0u, d.size());
}

TEST(interface, split_single_halffull_buffer_in_half) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 20));

  auto d = c.split(10);

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(10u, c.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());
}

TEST(interface, split_single_halffull_buffer_completely) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 20));

  auto d = c.split(20);

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0u, c.length());
  EXPECT_EQ(0u, c.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(20u, d.size());
}

TEST(interface, split_single_full_buffer_in_half) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 50));

  auto d = c.split(25);

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(25u, c.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(25u, d.size());
}

TEST(interface, split_single_full_buffer_completely) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 50));

  auto d = c.split(50);

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0u, c.length());
  EXPECT_EQ(0u, c.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(50u, d.size());
}

TEST(interface, split_single_full_buffer_completely_multistep) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 50));

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(50u, c.size());

  auto d = c.split(10);

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(40u, c.size());

  d = c.split(10);

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(30u, c.size());

  d = c.split(10);

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(20u, c.size());

  d = c.split(10);

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(1u, c.length());
  EXPECT_EQ(10u, c.size());

  d = c.split(10);

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0u, c.length());
  EXPECT_EQ(0u, c.size());
}

TEST(interface, split_multiple_halffull_buffers_in_half) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));

  auto d = c.split(30);

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(2u, c.length());
  EXPECT_EQ(30u, c.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(2u, d.length());
  EXPECT_EQ(30u, d.size());
}

TEST(interface, split_multiple_halffull_buffers_completely) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));
  c.push_back(make_buffer(50, 20));

  auto d = c.split(60);

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0u, c.length());
  EXPECT_EQ(0u, c.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(3u, d.length());
  EXPECT_EQ(60u, d.size());
}

TEST(interface, split_multiple_full_buffers_in_half) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 50));
  c.push_back(make_buffer(50, 50));
  c.push_back(make_buffer(50, 50));

  auto d = c.split(75);

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(2u, c.length());
  EXPECT_EQ(75u, c.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(2u, d.length());
  EXPECT_EQ(75u, d.size());
}

TEST(interface, split_multiple_full_buffers_completely) {
  byte_buffer::chain c;
  c.push_back(make_buffer(50, 50));
  c.push_back(make_buffer(50, 50));
  c.push_back(make_buffer(50, 50));

  auto d = c.split(150);

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0u, c.length());
  EXPECT_EQ(0u, c.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(3u, d.length());
  EXPECT_EQ(150u, d.size());
}
