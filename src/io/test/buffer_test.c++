#include "xi/io/basic_buffer_allocator.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_reader.h"
#include "xi/io/detail/heap_buffer_storage_allocator.h"

#include <gtest/gtest.h>

#include "src/test/util.h"

using namespace xi;
using namespace xi::io;
using namespace xi::io::detail;

auto ALLOC = make< basic_buffer_allocator< heap_buffer_storage_allocator > >();

auto
make_buffer(usize headroom, usize data, usize tailroom) {
  vector< u8 > in(data);
  usize i = 0u;
  std::generate_n(begin(in), data, [&] { return ++i; });

  auto b = ALLOC->allocate(data, headroom, tailroom);
  b.write(byte_range{in});
  return move(b);
}

auto
make_buffer(string data, usize headroom = 0, usize tailroom = 0) {
  auto b = ALLOC->allocate(data.size(), headroom, tailroom);
  b.write(byte_range{data});
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

TEST(interface, push_back_empty_buffer_skipd) {
  buffer b;

  b.push_back(make_buffer(0, 0, 0));

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
  EXPECT_EQ(0u, b.tailroom());
}

TEST(interface, skip_bytes_empty_buffer) {
  buffer b;

  b.skip_bytes(0);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
  EXPECT_EQ(0u, b.tailroom());

  b.skip_bytes(1);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
  EXPECT_EQ(0u, b.tailroom());

  b.skip_bytes(100);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
  EXPECT_EQ(0u, b.tailroom());
}

TEST(interface, skip_bytes_single_buffer_half) {
  buffer b;

  b.push_back(make_buffer(10,20,30));
  b.skip_bytes(10);

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(10u, b.size());
  EXPECT_EQ(20u, b.headroom());
  EXPECT_EQ(30u, b.tailroom());
}

TEST(interface, skip_bytes_single_buffer_completely) {
  buffer b;

  b.push_back(make_buffer(10,20,30));
  b.skip_bytes(20);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
  EXPECT_EQ(0u, b.headroom());
  EXPECT_EQ(0u, b.tailroom());
}

TEST(interface, skip_bytes_single_buffer_over) {
  buffer b;

  b.push_back(make_buffer(10,20,30));
  b.skip_bytes(200);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
  EXPECT_EQ(0u, b.headroom());
  EXPECT_EQ(0u, b.tailroom());
}

TEST(interface, skip_bytes_multi_buffer_half_first) {
  buffer b;

  b.push_back(make_buffer(10,20,30));
  b.push_back(make_buffer(10,20,30));
  b.skip_bytes(10);

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(2u, b.length());
  EXPECT_EQ(30u, b.size());
  EXPECT_EQ(20u, b.headroom());
  EXPECT_EQ(30u, b.tailroom());
}

TEST(interface, skip_bytes_multi_buffer_full_first) {
  buffer b;

  b.push_back(make_buffer(10,20,30));
  b.push_back(make_buffer(10,20,30));
  b.skip_bytes(20);

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(20u, b.size());
  EXPECT_EQ(10u, b.headroom());
  EXPECT_EQ(30u, b.tailroom());
}

TEST(interface, skip_bytes_multi_buffer_half_second) {
  buffer b;

  b.push_back(make_buffer(10,20,30));
  b.push_back(make_buffer(10,20,30));
  b.skip_bytes(30);

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(10u, b.size());
  EXPECT_EQ(20u, b.headroom());
  EXPECT_EQ(30u, b.tailroom());
}

TEST(interface, skip_bytes_multi_buffer_full_second) {
  buffer b;

  b.push_back(make_buffer(10,20,30));
  b.push_back(make_buffer(10,20,30));
  b.skip_bytes(40);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
  EXPECT_EQ(0u, b.headroom());
  EXPECT_EQ(0u, b.tailroom());
}

TEST(interface, skip_bytes_multi_buffer_over) {
  buffer b;

  b.push_back(make_buffer(10,20,30));
  b.push_back(make_buffer(10,20,30));
  b.skip_bytes(400);

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
  EXPECT_EQ(0u, b.headroom());
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
    EXPECT_EQ(i - 1u, r.find_byte(i).unwrap());
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
    EXPECT_EQ(i - 1u, r.find_byte(i).unwrap());
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
    EXPECT_EQ(i - 1u, r.find_byte(i).unwrap());
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
    EXPECT_EQ(i - 1u, r.find_byte(i).unwrap());
  }

  for (u8 i = 1; i <= 40; ++i) {
    EXPECT_EQ(49u, r.find_byte(i, i).unwrap());
  }

  for (u8 i = 41; i <= 50; ++i) {
    EXPECT_EQ(none, r.find_byte(i, i));
  }
}

TEST(reader, read_value_and_skip_single_buffer) {
  buffer b = make_buffer("ABC");

  auto r = make_reader(edit(b));
  EXPECT_EQ('A', r.read_value_and_skip<char>().unwrap());
  EXPECT_EQ('B', r.read_value_and_skip<char>().unwrap());
  EXPECT_EQ('C', r.read_value_and_skip<char>().unwrap());
  EXPECT_EQ(none, r.read_value_and_skip<char>());
}

TEST(reader, read_value_and_skip_mutliple_buffers) {
  buffer b = make_buffer("ABC");
  b.push_back(make_buffer("DEF"));

  auto r = make_reader(edit(b));
  EXPECT_EQ('A', r.read_value_and_skip<char>().unwrap());
  EXPECT_EQ('B', r.read_value_and_skip<char>().unwrap());
  EXPECT_EQ('C', r.read_value_and_skip<char>().unwrap());
  EXPECT_EQ('D', r.read_value_and_skip<char>().unwrap());
  EXPECT_EQ('E', r.read_value_and_skip<char>().unwrap());
  EXPECT_EQ('F', r.read_value_and_skip<char>().unwrap());
  EXPECT_EQ(none, r.read_value_and_skip<char>());
}

TEST(reader, read_value_and_mark_single_buffer) {
  buffer b = make_buffer("ABC");

  auto r = make_reader(edit(b));
  EXPECT_EQ('A', r.read_value_and_mark<char>().unwrap());
  EXPECT_EQ('B', r.read_value_and_mark<char>().unwrap());
  EXPECT_EQ('C', r.read_value_and_mark<char>().unwrap());
  EXPECT_EQ(none, r.read_value_and_mark<char>());
  EXPECT_EQ("ABC", r.consume_mark_into_string());
}

TEST(reader, read_value_and_mark_mutliple_buffers) {
  buffer b = make_buffer("ABC");
  b.push_back(make_buffer("DEF"));

  auto r = make_reader(edit(b));
  EXPECT_EQ('A', r.read_value_and_mark<char>().unwrap());
  EXPECT_EQ('B', r.read_value_and_mark<char>().unwrap());
  EXPECT_EQ("AB", r.consume_mark_into_string());
  EXPECT_EQ('C', r.read_value_and_mark<char>().unwrap());
  EXPECT_EQ('D', r.read_value_and_mark<char>().unwrap());
  EXPECT_EQ("CD", r.consume_mark_into_string());
  EXPECT_EQ('E', r.read_value_and_mark<char>().unwrap());
  EXPECT_EQ('F', r.read_value_and_mark<char>().unwrap());
  EXPECT_EQ(none, r.read_value_and_mark<char>());
  EXPECT_EQ("EF", r.consume_mark_into_string());
}

TEST(reader, skip_any_of_and_mark_single_buffer) {
  buffer b = make_buffer("ABC");

  auto r = make_reader(edit(b));
  EXPECT_EQ(0u, r.skip_any_of_and_mark(""));
  EXPECT_EQ(0u, r.skip_any_of_and_mark("BC"));// starts with A
  EXPECT_EQ(1u, r.skip_any_of_and_mark("A"));
  EXPECT_EQ(0u, r.skip_any_of_and_mark("AC"));// starts with B
  EXPECT_EQ(1u, r.skip_any_of_and_mark("ABc"));
  EXPECT_EQ(0u, r.skip_any_of_and_mark("AB"));// starts with C
  EXPECT_EQ(1u, r.skip_any_of_and_mark("ABC"));
  EXPECT_EQ("ABC", r.consume_mark_into_string());
}

TEST(reader, skip_any_of_and_mark_single_buffer_all_at_once) {
  buffer b = make_buffer("ABC");

  auto r = make_reader(edit(b));
  EXPECT_EQ(3u, r.skip_any_of_and_mark("CBA"));
  EXPECT_EQ("ABC", r.consume_mark_into_string());
}

TEST(reader, skip_any_of_and_mark_multiple_buffers) {
  buffer b = make_buffer("ABC");
  b.push_back(make_buffer("DEF"));

  auto r = make_reader(edit(b));
  EXPECT_EQ(0u, r.skip_any_of_and_mark(""));
  EXPECT_EQ(0u, r.skip_any_of_and_mark("aBCDEF"));// starts with A
  EXPECT_EQ(1u, r.skip_any_of_and_mark("AbCDEF"));
  EXPECT_EQ(0u, r.skip_any_of_and_mark("AbCDEF"));// starts with B
  EXPECT_EQ(1u, r.skip_any_of_and_mark("ABcDEF"));
  EXPECT_EQ(0u, r.skip_any_of_and_mark("ABcDEF"));// starts with C
  EXPECT_EQ(1u, r.skip_any_of_and_mark("ABCdEF"));
  EXPECT_EQ(0u, r.skip_any_of_and_mark("ABCdEF"));// starts with D
  EXPECT_EQ(1u, r.skip_any_of_and_mark("ABCDeF"));
  EXPECT_EQ(0u, r.skip_any_of_and_mark("ABCDeF"));// starts with E
  EXPECT_EQ(1u, r.skip_any_of_and_mark("ABCDEf"));
  EXPECT_EQ(0u, r.skip_any_of_and_mark("ABCDEf"));// starts with F
  EXPECT_EQ(1u, r.skip_any_of_and_mark("ABCDEF"));
  EXPECT_EQ("ABCDEF", r.consume_mark_into_string());
}

TEST(reader, skip_any_not_of_and_mark_single_buffer_empty_pattern) {
  buffer b = make_buffer("ABC");

  auto r = make_reader(edit(b));
  EXPECT_EQ(3u, r.skip_any_not_of_and_mark(""));
  EXPECT_EQ("ABC", r.consume_mark_into_string());
}

TEST(reader, skip_any_not_of_and_mark_single_buffer) {
  buffer b = make_buffer("ABC");

  auto r = make_reader(edit(b));
  EXPECT_EQ(0u, r.skip_any_not_of_and_mark("A"));// starts with A
  EXPECT_EQ(1u, r.skip_any_not_of_and_mark("BC"));
  EXPECT_EQ(0u, r.skip_any_not_of_and_mark("B"));// starts with B
  EXPECT_EQ(1u, r.skip_any_not_of_and_mark("AC"));
  EXPECT_EQ(0u, r.skip_any_not_of_and_mark("C"));// starts with C
  EXPECT_EQ(1u, r.skip_any_not_of_and_mark("AB"));
  EXPECT_EQ("ABC", r.consume_mark_into_string());
}

TEST(reader, skip_any_not_of_and_mark_single_buffer_all_at_once) {
  buffer b = make_buffer("ABC");

  auto r = make_reader(edit(b));
  EXPECT_EQ(3u, r.skip_any_not_of_and_mark("DEF"));
  EXPECT_EQ("ABC", r.consume_mark_into_string());
}

TEST(reader, skip_any_not_of_and_mark_multiple_buffers_empty_pattern) {
  buffer b = make_buffer("ABC");
  b.push_back(make_buffer("DEF"));

  auto r = make_reader(edit(b));
  EXPECT_EQ(6u, r.skip_any_not_of_and_mark(""));
  EXPECT_EQ("ABCDEF", r.consume_mark_into_string());
}

TEST(reader, skip_any_not_of_and_mark_multiple_buffers) {
  buffer b = make_buffer("ABC");
  b.push_back(make_buffer("DEF"));

  auto r = make_reader(edit(b));
  EXPECT_EQ(0u, r.skip_any_not_of_and_mark("Abcdef"));// starts with A
  EXPECT_EQ(1u, r.skip_any_not_of_and_mark("aBCDEF"));
  EXPECT_EQ(0u, r.skip_any_not_of_and_mark("aBcdef"));// starts with B
  EXPECT_EQ(1u, r.skip_any_not_of_and_mark("AbCDEF"));
  EXPECT_EQ(0u, r.skip_any_not_of_and_mark("abCdef"));// starts with C
  EXPECT_EQ(1u, r.skip_any_not_of_and_mark("ABcDEF"));
  EXPECT_EQ(0u, r.skip_any_not_of_and_mark("abcDef"));// starts with D
  EXPECT_EQ(1u, r.skip_any_not_of_and_mark("ABCdEF"));
  EXPECT_EQ(0u, r.skip_any_not_of_and_mark("abcdEf"));// starts with E
  EXPECT_EQ(1u, r.skip_any_not_of_and_mark("ABCDeF"));
  EXPECT_EQ(0u, r.skip_any_not_of_and_mark("abcdeF"));// starts with F
  EXPECT_EQ(1u, r.skip_any_not_of_and_mark("ABCDEf"));
  EXPECT_EQ("ABCDEF", r.consume_mark_into_string());
}

TEST(reader, skip_any_of_and_mark_multiple_buffers_all_at_once) {
  buffer b = make_buffer("ABC");
  b.push_back(make_buffer("DEF"));

  auto r = make_reader(edit(b));
  EXPECT_EQ(6u, r.skip_any_not_of_and_mark("GHIJKLMNOPQRSTUVWXYZ"));
  EXPECT_EQ("ABCDEF", r.consume_mark_into_string());
}

TEST(reader, discard_to_mark_single_buffer) {
  buffer b = make_buffer("ABC");

  auto r = make_reader(edit(b));
  EXPECT_EQ(3u, b.size());
  EXPECT_EQ(0u, b.headroom());

  EXPECT_EQ(2u, r.skip_any_of_and_mark("BA"));
  EXPECT_EQ(3u, b.size());
  EXPECT_EQ(0u, b.headroom());

  r.discard_to_mark();
  EXPECT_EQ(1u, b.size());
  EXPECT_EQ(1u, r.total_size());
  EXPECT_EQ(2u, b.headroom());

  EXPECT_EQ(1u, r.skip_any_of_and_mark("C"));
  r.discard_to_mark();

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, r.total_size());
  EXPECT_EQ(0u, r.unmarked_size());
  EXPECT_EQ(0u, r.marked_size());
}

TEST(reader, discard_to_mark_multiple_buffers) {
  buffer b = make_buffer("ABC");
  b.push_back(make_buffer("DEF"));

  auto r = make_reader(edit(b));
  EXPECT_EQ(6u, b.size());
  EXPECT_EQ(0u, b.headroom());

  EXPECT_EQ(2u, r.skip_any_of_and_mark("BA"));
  EXPECT_EQ(6u, b.size());
  EXPECT_EQ(0u, b.headroom());

  r.discard_to_mark();
  EXPECT_EQ(4u, b.size());
  EXPECT_EQ(4u, r.total_size());
  EXPECT_EQ(2u, b.headroom());

  EXPECT_EQ(2u, r.skip_any_of_and_mark("CD"));
  r.discard_to_mark();

  EXPECT_EQ(2u, b.size());
  EXPECT_EQ(1u, b.length());
  EXPECT_EQ(2u, r.total_size());
  EXPECT_EQ(1u, b.headroom());

  EXPECT_EQ(2u, r.skip_any_of_and_mark("EF"));
  r.discard_to_mark();

  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, r.total_size());
  EXPECT_EQ(0u, r.unmarked_size());
  EXPECT_EQ(0u, r.marked_size());
}
