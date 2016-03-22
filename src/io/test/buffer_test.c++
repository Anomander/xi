#include "xi/io/buffer.h"
#include "xi/io/buffer_reader.h"
#include "xi/io/basic_buffer_allocator.h"
#include "xi/io/detail/heap_buffer_storage_allocator.h"

#include <gtest/gtest.h>

#include "src/test/util.h"

using namespace xi;
using namespace xi::io;
using namespace xi::io::detail;

auto ALLOC = make< basic_buffer_allocator< heap_buffer_storage_allocator > >();

auto make_buffer(usize headroom, usize data, usize tailroom) {
  vector< u8 > in(data);
  usize i = 0u;
  std::generate_n(begin(in), data, [&] { return ++i; });

  auto b = ALLOC->allocate(data, headroom, tailroom);
  b.write(byte_range{in});
  return move(b);
}

TEST(interface, empty_chain) {
  buffer b;
  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
  EXPECT_EQ(0u, b.tailroom());
}

TEST(interface, push_back_changes_stats) {
  buffer b;

  b.push_back(make_buffer(50, 50, 50));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(50u, b.size());
  EXPECT_EQ(50u, b.headroom());
  EXPECT_EQ(50u, b.tailroom());

  b.push_back(make_buffer(0, 50, 10));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(2u, b.length());
  EXPECT_EQ(100u, b.size());
  EXPECT_EQ(50u, b.headroom());
  EXPECT_EQ(10u, b.tailroom());

  b.push_back(make_buffer(10, 50, 20));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(3u, b.length());
  EXPECT_EQ(150u, b.size());
  EXPECT_EQ(50u, b.headroom());
  EXPECT_EQ(20u, b.tailroom());
}

TEST(interface, push_back_empty_buffer_ignored) {
  buffer b;

  b.push_back(make_buffer(0, 0, 0));

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
  EXPECT_EQ(0u, b.tailroom());
}

TEST(interface, coalesce_empty_chain) {
  buffer b;

  EXPECT_EQ(0u, b.coalesce(edit(ALLOC)));

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
}

TEST(interface, coalesce_capped_empty_chain) {
  buffer b;

  EXPECT_EQ(0u, b.coalesce(edit(ALLOC), 0, 1));

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
}

TEST(interface, coalesce_single_empty_buffer) {
  buffer b;
  b.push_back(make_buffer(0, 0, 0));

  EXPECT_EQ(0u, b.coalesce(edit(ALLOC)));

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());

  b.push_back(make_buffer(0, 0, 0));

  EXPECT_EQ(0u, b.coalesce(edit(ALLOC)));

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
}

TEST(interface, coalesce_capped_single_empty_buffer) {
  buffer b;

  EXPECT_EQ(0u, b.coalesce(edit(ALLOC), 0, 25));

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());

  b.push_back(make_buffer(0, 0, 0));

  EXPECT_EQ(0u, b.coalesce(edit(ALLOC), 0, 25));

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
}

TEST(interface, coalesce_single_halffull_buffer) {
  buffer b;
  b.push_back(make_buffer(0, 50, 25));

  EXPECT_EQ(50u, b.coalesce(edit(ALLOC)));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(50u, b.size());
}

TEST(interface, coalesce_capped_single_halffull_buffer) {
  buffer b;
  b.push_back(make_buffer(0, 20, 25));

  EXPECT_EQ(20u, b.coalesce(edit(ALLOC), 0, 10));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(20u, b.size());

  EXPECT_EQ(20u, b.coalesce(edit(ALLOC), 0, 25));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(20u, b.size());
}

TEST(interface, coalesce_single_full_buffer) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));

  EXPECT_EQ(50u, b.coalesce(edit(ALLOC)));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(50u, b.size());
}

TEST(interface, coalesce_capped_single_full_buffer) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));

  EXPECT_EQ(50u, b.coalesce(edit(ALLOC), 0, 10));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(50u, b.size());

  EXPECT_EQ(50u, b.coalesce(edit(ALLOC), 0, 25));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(50u, b.size());

  EXPECT_EQ(50u, b.coalesce(edit(ALLOC), 0, 50));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(50u, b.size());
}

TEST(interface, coalesce_multiple_enough_storage) {
  buffer b;
  b.push_back(make_buffer(0, 10, 40));
  b.push_back(make_buffer(0, 10, 0));
  b.push_back(make_buffer(0, 10, 0));
  b.push_back(make_buffer(0, 10, 0));
  b.push_back(make_buffer(0, 10, 0));

  EXPECT_EQ(5u, b.length());

  EXPECT_EQ(50u, b.coalesce(edit(ALLOC)));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(50u, b.size());
}

TEST(interface, coalesce_capped_multiple_enough_storage) {
  buffer b;
  b.push_back(make_buffer(0, 10, 15));
  b.push_back(make_buffer(0, 10, 0));
  b.push_back(make_buffer(0, 10, 0));
  b.push_back(make_buffer(0, 10, 0));
  b.push_back(make_buffer(0, 10, 0));

  EXPECT_EQ(5u, b.length());
  EXPECT_EQ(50u, b.size());

  EXPECT_EQ(25u, b.coalesce(edit(ALLOC), 0, 25));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(4u, b.length());
  EXPECT_EQ(50u, b.size());
}

TEST(interface, coalesce_multiple_not_enough_storage) {
  buffer b;
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));

  EXPECT_EQ(5u, b.length());

  EXPECT_EQ(100u, b.coalesce(edit(ALLOC)));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(100u, b.size());
}

TEST(interface, coalesce_capped_multiple_not_enough_storage) {
  buffer b;
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));

  EXPECT_EQ(5u, b.length());

  EXPECT_EQ(50u, b.coalesce(edit(ALLOC), 0, 50));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(4u, b.length());
  EXPECT_EQ(100u, b.size());
}

TEST(interface, split_empty_chain) {
  buffer b;

  auto d = b.split(1);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());

  EXPECT_TRUE(d.empty());
  EXPECT_EQ(0u, d.length());
  EXPECT_EQ(0u, d.size());
}

TEST(interface, split_single_empty_buffer) {
  buffer b;
  b.push_back(make_buffer(0, 0, 50));

  auto d = b.split(1);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(0u, d.size());
}

TEST(interface, split_multiple_empty_buffers) {
  buffer b;
  b.push_back(make_buffer(0, 0, 50));
  b.push_back(make_buffer(0, 0, 50));
  b.push_back(make_buffer(0, 0, 50));

  auto d = b.split(1);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(3u, d.length());
  EXPECT_EQ(0u, d.size());
}

TEST(interface, split_single_halffull_buffer_in_half) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));

  auto d = b.split(25);

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(25u, b.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(25u, d.size());
}

TEST(interface, split_single_halffull_buffer_completely) {
  buffer b;
  b.push_back(make_buffer(0, 50, 20));

  auto d = b.split(50);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(50u, d.size());
}

TEST(interface, split_single_full_buffer_in_half) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));

  auto d = b.split(25);

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(25u, b.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(25u, d.size());
}

TEST(interface, split_single_full_buffer_completely) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));

  auto d = b.split(50);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(50u, d.size());
}

TEST(interface, split_single_full_buffer_completely_multistep) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(50u, b.size());

  auto d = b.split(10);

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(40u, b.size());

  d = b.split(10);

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(30u, b.size());

  d = b.split(10);

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(20u, b.size());

  d = b.split(10);

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(10u, b.size());

  d = b.split(10);

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(1u, d.length());
  EXPECT_EQ(10u, d.size());

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
}

TEST(interface, split_multiple_halffull_buffers_in_half) {
  buffer b;
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));

  auto d = b.split(30);

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(2u, b.length());
  EXPECT_EQ(30u, b.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(2u, d.length());
  EXPECT_EQ(30u, d.size());
}

TEST(interface, split_multiple_halffull_buffers_completely) {
  buffer b;
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));
  b.push_back(make_buffer(0, 20, 20));

  auto d = b.split(60);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(3u, d.length());
  EXPECT_EQ(60u, d.size());
}

TEST(interface, split_multiple_full_buffers_in_half) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));
  b.push_back(make_buffer(0, 50, 50));
  b.push_back(make_buffer(0, 50, 50));

  auto d = b.split(75);

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(2u, b.length());
  EXPECT_EQ(75u, b.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(2u, d.length());
  EXPECT_EQ(75u, d.size());
}

TEST(interface, split_multiple_full_buffers_completely) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));
  b.push_back(make_buffer(0, 50, 50));
  b.push_back(make_buffer(0, 50, 50));

  auto d = b.split(150);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());

  EXPECT_FALSE(d.empty());
  EXPECT_EQ(3u, d.length());
  EXPECT_EQ(150u, d.size());
}

TEST(reader, find_byte_empty_buffer) {
  buffer b;
  auto r = make_reader(edit(b));

  EXPECT_EQ(none, r.find_byte(1));
  EXPECT_EQ(none, r.find_byte(1, 100));
}

TEST(reader, find_byte_single_buffer) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));
  auto r = make_reader(edit(b));

  for (u8 i = 1; i <= 50; ++i) {
    EXPECT_EQ(i-1u, r.find_byte(i).unwrap());
  }
  for (u8 i = 1; i <= 50; ++i) {
    EXPECT_EQ(none, r.find_byte(i, i));
  }
}

TEST(reader, find_byte_multiple_buffers) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));
  b.push_back(make_buffer(0, 50, 50));
  auto r = make_reader(edit(b));

  for (u8 i = 1; i <= 50; ++i) {
    EXPECT_EQ(i-1u, r.find_byte(i).unwrap());
  }

  for (u8 i = 1; i <= 50; ++i) {
    EXPECT_EQ(49u, r.find_byte(i, i).unwrap());
  }
}

TEST(reader, find_byte_single_buffer_boundaries) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));
  b.ignore_bytes_at_end(10);
  EXPECT_EQ(40u, b.size());
  auto r = make_reader(edit(b));

  for (u8 i = 1; i <= 40; ++i) {
    EXPECT_EQ(i-1u, r.find_byte(i).unwrap());
  }

  for (u8 i = 41; i <= 50; ++i) {
    EXPECT_EQ(none, r.find_byte(i));
  }
}

TEST(reader, find_byte_multiple_buffers_boundaries) {
  buffer b;
  b.push_back(make_buffer(0, 50, 50));
  b.push_back(make_buffer(0, 50, 50));

  b.ignore_bytes_at_end(10);
  EXPECT_EQ(90u, b.size());
  auto r = make_reader(edit(b));

  for (u8 i = 1; i <= 50; ++i) {
    EXPECT_EQ(i-1u, r.find_byte(i).unwrap());
  }

  for (u8 i = 1; i <= 40; ++i) {
    EXPECT_EQ(49u, r.find_byte(i,i).unwrap());
  }

  for (u8 i = 41; i <= 50; ++i) {
    EXPECT_EQ(none, r.find_byte(i, i));
  }
}
