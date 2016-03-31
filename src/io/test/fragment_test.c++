#include "xi/io/basic_buffer_allocator.h"
#include "xi/io/detail/heap_buffer_storage_allocator.h"
#include "xi/io/fragment.h"

#include <gtest/gtest.h>

#include "src/test/util.h"

using namespace xi;
using namespace xi::io;
using namespace xi::io::detail;

auto ALLOC = make< buffer_arena_allocator< heap_buffer_storage_allocator > >();

auto
make_fragment(usize headroom, usize data, usize tailroom) {
  vector< u8 > in(data);
  usize i = 0u;
  std::generate_n(begin(in), data, [&] { return ++i; });

  auto size  = data + headroom + tailroom;
  auto arena = ALLOC->allocate_arena(size);
  auto b     = new fragment(arena, arena->allocate(size), size);
  if (headroom) {
    b->advance(headroom);
  }
  b->write(byte_range{in});
  arena->consume(size);
  return unique_ptr< fragment >(b);
}

TEST(interface, create) {
  auto b = make_fragment(1024, 0, 1024);

  ASSERT_EQ(1024U, b->headroom());
  ASSERT_EQ(0U, b->size());
  ASSERT_EQ(1024U, b->tailroom());
}

TEST(interface, read_from_empty_does_nothing) {
  auto b = make_fragment(0, 0, 1024);
  vector< u8 > v(10);

  auto ret = b->read(byte_range{v});
  ASSERT_EQ(0U, ret);

  ret = b->read(byte_range{v}, 1);
  ASSERT_EQ(0U, ret);
}

TEST(interface, read_offset) {
  auto b = make_fragment(0, 0, 10);
  vector< u8 > in1 = {0, 1, 2, 3, 4}, in2 = {5, 6, 7, 8, 9}, out(5);

  b->write(byte_range{in1});
  b->write(byte_range{in2});
  EXPECT_EQ(10u, b->size());

  auto ret = b->read(byte_range{out}, 0);
  ASSERT_EQ(5u, ret);
  EXPECT_EQ(in1, out);

  ret = b->read(byte_range{out}, 5);
  ASSERT_EQ(5u, ret);
  EXPECT_EQ(in2, out);

  ret = b->read(byte_range{out}, 3);
  ASSERT_EQ(5u, ret);
  EXPECT_EQ(3, out[0]);
  EXPECT_EQ(4, out[1]);
  EXPECT_EQ(5, out[2]);
  EXPECT_EQ(6, out[3]);
  EXPECT_EQ(7, out[4]);
}

TEST(interface, write_changes_stats) {
  auto b          = make_fragment(0, 0, 1024);
  vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  b->write(byte_range{in});
  ASSERT_EQ(1014U, b->tailroom());
  ASSERT_EQ(10U, b->size());
}

TEST(interface, written_can_be_read) {
  auto b          = make_fragment(0, 0, 1024);
  vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, out(10);

  auto ret = b->write(byte_range{in});
  ASSERT_EQ(10U, ret);
  ret = b->read(byte_range{out});
  ASSERT_EQ(10U, ret);
  ASSERT_EQ(in, out);
}

TEST(interface, reads_repeat) {
  auto b          = make_fragment(0, 0, 1024);
  vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, out1(10), out2(10);

  auto ret = b->write(byte_range{in});
  ASSERT_EQ(10U, ret);

  ret = b->read(byte_range{out1});
  ASSERT_EQ(10U, ret);
  ASSERT_EQ(in, out1);

  ret = b->read(byte_range{out2});
  ASSERT_EQ(10U, ret);
  ASSERT_EQ(in, out2);

  ASSERT_EQ(out1, out2);
}

TEST(interface, skip_bytes) {
  auto b          = make_fragment(0, 0, 1024);
  vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  auto ret = b->write(byte_range{in});
  ASSERT_EQ(10U, ret);

  ASSERT_EQ(1014U, b->tailroom());
  ASSERT_EQ(10U, b->size());

  ret = b->skip_bytes(ret);
  ASSERT_EQ(10U, ret);
  ASSERT_EQ(10U, b->headroom());
  ASSERT_EQ(1014U, b->tailroom());
  ASSERT_EQ(0U, b->size());
}

TEST(interface, clone) {
  auto b = make_fragment(1024, 0, 1024);
  ASSERT_EQ(1024U, b->headroom());
  ASSERT_EQ(1024U, b->tailroom());
  ASSERT_EQ(0U, b->size());

  auto c = b->clone();
  ASSERT_EQ(1024U, c->headroom());
  ASSERT_EQ(1024U, c->tailroom());
  ASSERT_EQ(0U, c->size());
}

TEST(interface, advance_empty_buffer) {
  auto b = make_fragment(0, 0, 1024);
  ASSERT_EQ(1024U, b->tailroom());
  ASSERT_EQ(0U, b->size());

  b->advance(0);
  ASSERT_EQ(1024U, b->tailroom());
  ASSERT_EQ(0U, b->size());
}

TEST(interface, split_empty_buffer) {
  auto b = make_fragment(0, 0, 0);

  EXPECT_EQ(nullptr, b->split(0));
}

TEST(interface, split_buffer_no_tailroom) {
  auto b = make_fragment(10, 50, 0);
  EXPECT_EQ(10u, b->headroom());
  EXPECT_EQ(50u, b->size());
  EXPECT_EQ(0u, b->tailroom());

  auto c = b->split(20);
  EXPECT_EQ(0u, c->headroom());
  EXPECT_EQ(30u, c->size());
  EXPECT_EQ(0u, c->tailroom());

  EXPECT_EQ(10u, b->headroom());
  EXPECT_EQ(20u, b->size());
  EXPECT_EQ(0u, b->tailroom());
}

TEST(interface, split_buffer_no_headroom) {
  auto b = make_fragment(0, 50, 10);
  EXPECT_EQ(0u, b->headroom());
  EXPECT_EQ(50u, b->size());
  EXPECT_EQ(10u, b->tailroom());

  auto c = b->split(20);
  EXPECT_EQ(0u, c->headroom());
  EXPECT_EQ(30u, c->size());
  EXPECT_EQ(10u, c->tailroom());

  EXPECT_EQ(0u, b->headroom());
  EXPECT_EQ(20u, b->size());
  EXPECT_EQ(0u, b->tailroom());
}

TEST(interface, split_buffer_neither_headroom_nor_tailroom) {
  auto b = make_fragment(0, 50, 0);
  EXPECT_EQ(0u, b->headroom());
  EXPECT_EQ(50u, b->size());
  EXPECT_EQ(0u, b->tailroom());

  auto c = b->split(20);
  EXPECT_EQ(0u, c->headroom());
  EXPECT_EQ(30u, c->size());
  EXPECT_EQ(0u, c->tailroom());

  EXPECT_EQ(0u, b->headroom());
  EXPECT_EQ(20u, b->size());
  EXPECT_EQ(0u, b->tailroom());
}

TEST(interface, split_buffer_complete_no_tailroom) {
  auto b = make_fragment(0, 50, 0);
  EXPECT_EQ(0u, b->headroom());
  EXPECT_EQ(50u, b->size());
  EXPECT_EQ(0u, b->tailroom());

  EXPECT_EQ(nullptr, b->split(50));
}

TEST(interface, split_buffer_complete_with_tailroom) {
  auto b = make_fragment(0, 50, 10);
  EXPECT_EQ(0u, b->headroom());
  EXPECT_EQ(50u, b->size());
  EXPECT_EQ(10u, b->tailroom());

  EXPECT_EQ(nullptr, b->split(50));
  EXPECT_EQ(0u, b->headroom());
  EXPECT_EQ(50u, b->size());
  EXPECT_EQ(10u, b->tailroom());
}
