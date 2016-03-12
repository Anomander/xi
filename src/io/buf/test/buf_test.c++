#include "xi/io/byte_buffer.h"
#include "xi/io/basic_byte_buffer_allocator.h"
#include "xi/io/detail/heap_byte_buffer_storage_allocator.h"

#include <netinet/in.h>
#include <gtest/gtest.h>

#include "src/test/util.h"

using namespace xi;
using namespace xi::io;
using namespace xi::io::detail;

auto ALLOC2 = make< basic_byte_buffer_allocator<heap_byte_buffer_storage_allocator> >();

// struct trackable_block : public block, public test::object_tracker {
//   using block::block;
// };

// struct trackable_heap_allocator : public heap_allocator {
//   block* allocate(size_t sz) override {
//     auto mem = malloc(sz + sizeof(trackable_block));
//     return new (mem) trackable_block{sz};
//   }
// };

// TEST(interface, buf_is_created_empty) {
//   buf b(ALLOC);

//   ASSERT_TRUE(b.empty());
//   ASSERT_EQ(0U, b.size());
//   ASSERT_EQ(0U, b.capacity());
// }

TEST(interface2, create) {
  auto b = ALLOC2->allocate(1024);

  ASSERT_EQ(0U, b->size());
  ASSERT_EQ(1024U, b->tailroom());
}

TEST(correctness2, read_from_empty_does_nothing) {
  auto b = ALLOC2->allocate(1024);
  vector< u8 > v(10);

  auto ret = b->read(byte_range{v});
  ASSERT_EQ(0U, ret);
}

TEST(correctness2, write_changes_stats) {
  auto b = ALLOC2->allocate(1024);
  vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  b->write(byte_range{in});
  ASSERT_EQ(1014U, b->tailroom());
  ASSERT_EQ(10U, b->size());
}

TEST(correctness2, written_can_be_read) {
  auto b = ALLOC2->allocate(1024);
  vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, out(10);

  auto ret = b->write(byte_range{in});
  ASSERT_EQ(10U, ret);
  ret = b->read(byte_range{out});
  ASSERT_EQ(10U, ret);
  ASSERT_EQ(in, out);
}

TEST(correctness2, reads_repeat) {
  auto b = ALLOC2->allocate(1024);
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

TEST(correctness2, discard_read) {
  auto b = ALLOC2->allocate(1024);
  vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  auto ret = b->write(byte_range{in});
  ASSERT_EQ(10U, ret);

  ASSERT_EQ(1014U, b->tailroom());
  ASSERT_EQ(10U, b->size());

  ret = b->discard_read(ret);
  ASSERT_EQ(10U, ret);
  ASSERT_EQ(10U, b->headroom());
  ASSERT_EQ(1014U, b->tailroom());
  ASSERT_EQ(0U, b->size());
}

TEST(correctness2, clone) {
  auto b = ALLOC2->allocate(1024, 1024);
  ASSERT_EQ(1024U, b->headroom());
  ASSERT_EQ(1024U, b->tailroom());
  ASSERT_EQ(0U, b->size());

  auto c = b->clone();
  ASSERT_EQ(1024U, c->headroom());
  ASSERT_EQ(1024U, c->tailroom());
  ASSERT_EQ(0U, c->size());
}


TEST(correctness2, advance_empty_buffer) {
  auto b = ALLOC2->allocate(1024);
  ASSERT_EQ(1024U, b->tailroom());
  ASSERT_EQ(0U, b->size());

  b->advance(0);
  ASSERT_EQ(1024U, b->tailroom());
  ASSERT_EQ(0U, b->size());
}

// TEST(interface, expand_buf) {
//   buf b(ALLOC);
//   b.expand(1024);

//   ASSERT_FALSE(b.empty());
//   ASSERT_EQ(0U, b.size());
//   ASSERT_EQ(1024U, b.capacity());
// }

// TEST(interface, double_expand_buf) {
//   buf b(ALLOC);
//   b.expand(1024);
//   b.expand(1024);

//   ASSERT_FALSE(b.empty());
//   ASSERT_EQ(0U, b.size());
//   ASSERT_EQ(2048U, b.capacity());
// }

// TEST(correctness, read_from_empty_does_nothing) {
//   buf b(ALLOC);
//   vector< u8 > v(10);

//   auto ret = b.read(v.data(), 10);
//   ASSERT_EQ(0U, ret);
// }

// TEST(correctness, write_to_empty_does_nothing) {
//   buf b(ALLOC);
//   vector< u8 > v(10);

//   auto ret = b.write(v.data(), 10);
//   ASSERT_EQ(0U, ret);
//   ASSERT_TRUE(b.empty());
//   ASSERT_EQ(0U, b.size());
//   ASSERT_EQ(0U, b.capacity());
// }

// TEST(write, changes_stats) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

//   b.expand(1024);
//   b.write(in.data(), 10);
//   ASSERT_EQ(1014U, b.capacity());
//   ASSERT_EQ(10U, b.size());
// }

// TEST(write, over_multiple_blocks_changes_stats) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

//   b.expand(5);
//   b.expand(5);
//   b.write(in.data(), 10);
//   ASSERT_EQ(0U, b.capacity());
//   ASSERT_EQ(10U, b.size());
// }

// TEST(correctness, written_can_be_read) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, out(10);

//   b.expand(1024);
//   auto ret = b.write(in.data(), 10);
//   ASSERT_EQ(10U, ret);
//   ret = b.read(out.data(), 10);
//   ASSERT_EQ(10U, ret);
//   ASSERT_EQ(in, out);
// }

// TEST(correctness, written_over_multiple_blocks_can_be_read) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, out(10);

//   b.expand(5);
//   b.expand(5);
//   auto ret = b.write(in.data(), 10);
//   ASSERT_EQ(10U, ret);
//   ret = b.read(out.data(), 10);
//   ASSERT_EQ(10U, ret);
//   ASSERT_EQ(in, out);
// }

// TEST(correctness, unconsumed_reads_repeat) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, out1(10), out2(10);

//   b.expand(1024);
//   auto ret = b.write(in.data(), 10);
//   ASSERT_EQ(10U, ret);

//   ret = b.read(out1.data(), 10);
//   ASSERT_EQ(10U, ret);
//   ASSERT_EQ(in, out1);

//   ret = b.read(out2.data(), 10);
//   ASSERT_EQ(10U, ret);
//   ASSERT_EQ(in, out2);

//   ASSERT_EQ(out1, out2);
// }

// TEST(correctness, consumed_reads_dont_repeat) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, out1(5), out2(5);
//   vector< u8 > out1_target = {0, 1, 2, 3, 4};
//   vector< u8 > out2_target = {5, 6, 7, 8, 9};

//   b.expand(1024);
//   auto ret = b.write(in.data(), 10);
//   ASSERT_EQ(10U, ret);

//   ret = b.read(out1.data(), 5);
//   ASSERT_EQ(5U, ret);
//   ASSERT_EQ(out1_target, out1);

//   b.consume(5);

//   ret = b.read(out2.data(), 5);
//   ASSERT_EQ(5U, ret);
//   ASSERT_EQ(out2_target, out2);
// }

// TEST(correctness, consumed_reads_dont_repeat_multiple_blocks) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, out1(5), out2(5);
//   vector< u8 > out1_target = {0, 1, 2, 3, 4};
//   vector< u8 > out2_target = {5, 6, 7, 8, 9};

//   b.expand(3);
//   b.expand(3);
//   b.expand(3);
//   b.expand(3);
//   auto ret = b.write(in.data(), 10);
//   ASSERT_EQ(10U, ret);

//   ret = b.read_consume(out1.data(), 5);
//   ASSERT_EQ(5U, ret);
//   ASSERT_EQ(out1_target, out1);

//   ret = b.read(out2.data(), 5);
//   ASSERT_EQ(5U, ret);
//   ASSERT_EQ(out2_target, out2);
// }

// TEST(correctness, consume_multiple_blocks) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, out1(5), out2(5);

//   b.expand(5);
//   b.expand(5);
//   b.expand(5);
//   b.expand(5);
//   auto ret = b.write(in.data(), 10);
//   ASSERT_EQ(10U, ret);
//   ret = b.write(in.data(), 10);
//   ASSERT_EQ(10U, ret);
//   ASSERT_EQ(20U, b.size());
//   ASSERT_EQ(0U, b.capacity());

//   b.consume(20);

//   ASSERT_EQ(0U, b.size());
//   ASSERT_EQ(0U, b.capacity());
// }

// TEST(allocations, dtor_releases_all_blocks) {
//   trackable_block::reset();
//   {
//     buf b(make< trackable_heap_allocator >());

//     ASSERT_EQ(0U, trackable_block::CREATED);
//     ASSERT_EQ(0U, trackable_block::DESTROYED);

//     b.reserve(100);
//     ASSERT_EQ(1U, trackable_block::CREATED);
//     ASSERT_EQ(0U, trackable_block::DESTROYED);

//     b.reserve(100); // already has capacity, no alloc
//     ASSERT_EQ(1U, trackable_block::CREATED);
//     ASSERT_EQ(0U, trackable_block::DESTROYED);

//     b.expand(100);
//     ASSERT_EQ(2U, trackable_block::CREATED);
//     ASSERT_EQ(0U, trackable_block::DESTROYED);
//   }
//   ASSERT_EQ(2U, trackable_block::DESTROYED);
// }

// TEST(correctness, reserve_only_adds_if_necessary) {
//   buf b(ALLOC);

//   b.expand(100);
//   ASSERT_EQ(0U, b.size());
//   ASSERT_EQ(100U, b.capacity());

//   b.reserve(99);
//   ASSERT_EQ(0U, b.size());
//   ASSERT_EQ(100U, b.capacity());

//   b.reserve(101);
//   ASSERT_EQ(0U, b.size());
//   ASSERT_EQ(101U, b.capacity());
// }

// TEST(iov, report_written_one_block) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

//   b.expand(100);
//   b.write(in.data(), in.size());

//   ASSERT_EQ(10U, b.size());
//   ASSERT_EQ(90U, b.capacity());

//   buf::buf::iovec_adapter::report_written(edit(b), 64);

//   ASSERT_EQ(74U, b.size());
//   ASSERT_EQ(26U, b.capacity());
// }

// TEST(iov, report_written_multuple_blocks) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

//   b.expand(5);
//   b.expand(10);
//   b.expand(85);
//   b.write(in.data(), in.size());

//   ASSERT_EQ(10U, b.size());
//   ASSERT_EQ(90U, b.capacity());

//   buf::buf::iovec_adapter::report_written(edit(b), 64);

//   ASSERT_EQ(74U, b.size());
//   ASSERT_EQ(26U, b.capacity());
// }

// TEST(iov, fill_iov_one_block) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

//   b.expand(100);
//   b.write(in.data(), in.size());

//   iovec iov[64];
//   auto ret = buf::buf::iovec_adapter::fill_writable_iovec(edit(b), iov, 64);

//   ASSERT_EQ(1U, ret);
//   ASSERT_EQ(90U, iov[0].iov_len);

//   ret = buf::buf::iovec_adapter::fill_readable_iovec(edit(b), iov, 64);

//   ASSERT_EQ(1U, ret);
//   ASSERT_EQ(10U, iov[0].iov_len);

//   auto out = vector< u8 >{(u8*)iov[0].iov_base,
//                           ((u8*)iov[0].iov_base + iov[0].iov_len)};
//   ASSERT_EQ(in, out);
// }

// TEST(iov, fill_iov_multiple_blocks) {
//   buf b(ALLOC);
//   vector< u8 > in = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

//   b.expand(5);
//   b.expand(10);
//   b.expand(5);
//   b.write(in.data(), in.size());

//   iovec iov[64];
//   auto ret = buf::buf::iovec_adapter::fill_writable_iovec(edit(b), iov, 64);

//   ASSERT_EQ(2U, ret);
//   ASSERT_EQ(5U, iov[0].iov_len);
//   ASSERT_EQ(5U, iov[1].iov_len);

//   ret = buf::buf::iovec_adapter::fill_readable_iovec(edit(b), iov, 64);

//   ASSERT_EQ(2U, ret);
//   ASSERT_EQ(5U, iov[0].iov_len);
//   ASSERT_EQ(5U, iov[1].iov_len);

//   auto out = vector< u8 >{(u8*)iov[0].iov_base,
//                           ((u8*)iov[0].iov_base + iov[0].iov_len)};
//   out.insert(end(out), (u8*)iov[1].iov_base,
//              ((u8*)iov[1].iov_base + iov[1].iov_len));
//   ASSERT_EQ(in, out);
// }
