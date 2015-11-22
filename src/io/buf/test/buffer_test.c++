#include "xi/io/buf/buffer.h"
#include "xi/io/buf/view.h"
#include "xi/io/buf/heap_buf.h"

#include <gtest/gtest.h>

#include "src/test/util.h"

using namespace xi;
using namespace xi::io;

struct trackable_heap_buf : public heap_buf, public test::object_tracker {
  using heap_buf::heap_buf;
};

TEST(assertions, buffer_larger_than_arena) {
  ASSERT_THROW(buffer(make< trackable_heap_buf >(10), 0, 11),
               std::out_of_range);
  ASSERT_THROW(buffer(make< trackable_heap_buf >(10), 1, 10),
               std::out_of_range);
  ASSERT_THROW(buffer(make< trackable_heap_buf >(10), 9, -7),
               std::out_of_range);
  ASSERT_THROW(buffer(make< trackable_heap_buf >(10), -6, -7),
               std::out_of_range);
  ASSERT_THROW(buffer(make< trackable_heap_buf >(10), 10, 1),
               std::out_of_range);
  ASSERT_THROW(buffer(make< trackable_heap_buf >(10), 11, -1),
               std::out_of_range);
}

TEST(assertions, buffer_fits_in_arena) {
  ASSERT_NO_THROW(buffer(make< trackable_heap_buf >(10), 0, 10));
  ASSERT_NO_THROW(buffer(make< trackable_heap_buf >(10), 1, 9));
  ASSERT_NO_THROW(buffer(make< trackable_heap_buf >(10), 9, 1));
  ASSERT_NO_THROW(buffer(make< trackable_heap_buf >(10), 10, 0));
}

TEST(interface, capacity_is_correct) {
  ASSERT_EQ(100UL, buffer(make< trackable_heap_buf >(1024), 0, 100).capacity());
  ASSERT_EQ(0UL, buffer(make< trackable_heap_buf >(1024), 0, 0).capacity());
  ASSERT_EQ(1024UL,
            buffer(make< trackable_heap_buf >(1024), 0, 1024).capacity());
}

TEST(interface, size_is_correct) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 100);
  ASSERT_EQ(0UL, buf.size());
  buf.append_tail(1);
  ASSERT_EQ(1UL, buf.size());
  buf.append_tail(10);
  ASSERT_EQ(11UL, buf.size());
  buf.append_tail(50);
  ASSERT_EQ(61UL, buf.size());
  buf.append_tail(39);
  ASSERT_EQ(100UL, buf.size());
  buf.append_tail(1);
  ASSERT_EQ(100UL, buf.size());
}

TEST(interface, is_empty_is_correct) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 10);
  ASSERT_TRUE(buf.is_empty());
  buf.append_tail(1);
  ASSERT_FALSE(buf.is_empty());
  buf.append_tail(9);
  ASSERT_FALSE(buf.is_empty());
  buf.consume_head(8);
  ASSERT_FALSE(buf.is_empty());
  buf.consume_head(2);
  ASSERT_TRUE(buf.is_empty());
}

TEST(interface, append_tail_is_correct) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 10);
  auto original_begin = buf.begin();
  auto original_end = buf.end();
  ASSERT_EQ(original_begin, original_end);

  ASSERT_EQ(0UL, buf.append_tail(1));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end + 1, buf.end());

  ASSERT_EQ(0UL, buf.append_tail(9));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());

  ASSERT_EQ(1UL, buf.append_tail(1));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());
}

TEST(interface, append_head_is_correct) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 10);
  auto original_begin = buf.begin();
  auto original_end = buf.end();
  ASSERT_EQ(original_begin, original_end);

  ASSERT_EQ(1UL, buf.append_head(1));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end, buf.end());

  buf.append_tail(10);
  buf.consume_head(10);
  ASSERT_EQ(original_begin + 10, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());

  ASSERT_EQ(0UL, buf.append_head(1));
  ASSERT_EQ(original_begin + 9, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());

  ASSERT_EQ(0UL, buf.append_head(9));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());

  ASSERT_EQ(1UL, buf.append_head(1));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());
}

TEST(interface, consume_head_is_correct) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 10);
  auto original_begin = buf.begin();
  auto original_end = buf.end();
  ASSERT_EQ(original_begin, original_end);

  ASSERT_EQ(1UL, buf.consume_head(1));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end, buf.end());

  buf.append_tail(10);
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());

  ASSERT_EQ(0UL, buf.consume_head(1));
  ASSERT_EQ(original_begin + 1, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());

  ASSERT_EQ(0UL, buf.consume_head(9));
  ASSERT_EQ(original_begin + 10, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());

  ASSERT_EQ(1UL, buf.consume_head(1));
  ASSERT_EQ(original_begin + 10, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());
}

TEST(interface, consume_tail_is_correct) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 10);
  auto original_begin = buf.begin();
  auto original_end = buf.end();
  ASSERT_EQ(original_begin, original_end);

  ASSERT_EQ(1UL, buf.consume_tail(1));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end, buf.end());

  buf.append_tail(10);
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end + 10, buf.end());

  ASSERT_EQ(0UL, buf.consume_tail(1));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end + 9, buf.end());

  ASSERT_EQ(0UL, buf.consume_tail(9));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end, buf.end());

  ASSERT_EQ(1UL, buf.consume_tail(1));
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(original_end, buf.end());
}

TEST(interface, headroom_is_correct) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 10);

  ASSERT_EQ(0UL, buf.headroom());

  buf.append_tail(10);
  ASSERT_EQ(0UL, buf.headroom());

  buf.consume_head(1);
  ASSERT_EQ(1UL, buf.headroom());

  buf.consume_head(9);
  ASSERT_EQ(10UL, buf.headroom());

  buf.consume_head(1);
  ASSERT_EQ(10UL, buf.headroom());

  buf.append_head(1);
  ASSERT_EQ(9UL, buf.headroom());

  buf.append_head(9);
  ASSERT_EQ(0UL, buf.headroom());

  buf.append_head(1);
  ASSERT_EQ(0UL, buf.headroom());
}

TEST(interface, tailroom_is_correct) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 10);

  ASSERT_EQ(10UL, buf.tailroom());

  buf.append_tail(10);
  ASSERT_EQ(0UL, buf.tailroom());

  buf.consume_tail(1);
  ASSERT_EQ(1UL, buf.tailroom());

  buf.consume_tail(9);
  ASSERT_EQ(10UL, buf.tailroom());

  buf.consume_tail(1);
  ASSERT_EQ(10UL, buf.tailroom());

  buf.append_tail(1);
  ASSERT_EQ(9UL, buf.tailroom());

  buf.append_tail(9);
  ASSERT_EQ(0UL, buf.tailroom());

  buf.append_tail(1);
  ASSERT_EQ(0UL, buf.tailroom());
}

TEST(interface, readable_head_is_correct) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 10);
  auto original_begin = buf.begin();
  ASSERT_EQ(make_pair(original_begin, 0UL), buf.readable_head());

  buf.append_tail(1);
  ASSERT_EQ(make_pair(original_begin, 1UL), buf.readable_head());

  buf.append_tail(9);
  ASSERT_EQ(make_pair(original_begin, 10UL), buf.readable_head());

  buf.consume_head(1);
  ASSERT_EQ(make_pair(original_begin + 1, 9UL), buf.readable_head());

  buf.consume_head(9);
  ASSERT_EQ(make_pair(original_begin + 10, 0UL), buf.readable_head());
}

TEST(interface, writable_tail_is_correct) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 10);
  auto original_end = buf.end();
  ASSERT_EQ(make_pair(original_end, 10UL), buf.writable_tail());

  buf.append_tail(1);
  ASSERT_EQ(make_pair(original_end+1, 9UL), buf.writable_tail());

  buf.append_tail(9);
  ASSERT_EQ(make_pair(original_end+10, 0UL), buf.writable_tail());

  buf.consume_tail(1);
  ASSERT_EQ(make_pair(original_end + 9, 1UL), buf.writable_tail());

  buf.consume_tail(9);
  ASSERT_EQ(make_pair(original_end, 10UL), buf.writable_tail());
}

TEST(lifetime, arena_is_destroyed_with_buffer) {
  trackable_heap_buf::reset();
  {
    auto buf = buffer(make< trackable_heap_buf >(1024), 0, 100);
    ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
    ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);
  }
  ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
  ASSERT_EQ(1UL, trackable_heap_buf::DESTROYED);
}

TEST(lifetime, view_prolongs_arena_lifetime) {
  trackable_heap_buf::reset();
  auto buf = make_unique_copy(buffer(make< trackable_heap_buf >(1024), 0, 100));
  {
    auto view = buf->make_view(0, 10);
    release(move(buf));
    ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
    ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);
  }
  ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
  ASSERT_EQ(1UL, trackable_heap_buf::DESTROYED);
}

TEST(lifetime, view_copies_prolong_arena_lifetime) {
  trackable_heap_buf::reset();
  auto buf = make_unique_copy(buffer(make< trackable_heap_buf >(1024), 0, 100));
  {
    auto view = make_unique_copy(buf->make_view(0, 10));
    {
      release(move(buf));
      ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
      ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);
    }
    auto view_copy = buffer::view(*view);
    ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
    ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);

    release(move(view));
    ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
    ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);
  }
  ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
  ASSERT_EQ(1UL, trackable_heap_buf::DESTROYED);
}

TEST(mutation, consume_head_less_than_size) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 100);
  buf.append_tail(100);
  auto original_begin = buf.begin();
  auto original_end = buf.end();
  ASSERT_EQ(0UL, buf.consume_head(1));
  ASSERT_EQ(99UL, buf.size());
  ASSERT_EQ(original_begin + 1, buf.begin());
  ASSERT_EQ(original_end, buf.end());

  ASSERT_EQ(0UL, buf.consume_head(10));
  ASSERT_EQ(89UL, buf.size());
  ASSERT_EQ(original_begin + 11, buf.begin());
  ASSERT_EQ(original_end, buf.end());

  ASSERT_EQ(0UL, buf.consume_head(50));
  ASSERT_EQ(39UL, buf.size());
  ASSERT_EQ(original_begin + 61, buf.begin());
  ASSERT_EQ(original_end, buf.end());

  ASSERT_EQ(0UL, buf.consume_head(39));
  ASSERT_EQ(0UL, buf.size());
  ASSERT_EQ(original_begin + 100, buf.begin());
  ASSERT_EQ(original_end, buf.end());
  ASSERT_EQ(buf.begin(), buf.end());
  ASSERT_TRUE(buf.is_empty());
}

TEST(mutation, consume_tail_less_than_size) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 100);
  buf.append_tail(100);
  auto original_begin = buf.begin();
  auto original_end = buf.end();
  ASSERT_EQ(0UL, buf.consume_tail(1));
  ASSERT_EQ(99UL, buf.size());
  ASSERT_EQ(original_end - 1, buf.end());
  ASSERT_EQ(original_begin, buf.begin());

  ASSERT_EQ(0UL, buf.consume_tail(10));
  ASSERT_EQ(89UL, buf.size());
  ASSERT_EQ(original_end - 11, buf.end());
  ASSERT_EQ(original_begin, buf.begin());

  ASSERT_EQ(0UL, buf.consume_tail(50));
  ASSERT_EQ(39UL, buf.size());
  ASSERT_EQ(original_end - 61, buf.end());
  ASSERT_EQ(original_begin, buf.begin());

  ASSERT_EQ(0UL, buf.consume_tail(39));
  ASSERT_EQ(0UL, buf.size());
  ASSERT_EQ(original_end - 100, buf.end());
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_EQ(buf.begin(), buf.end());
  ASSERT_TRUE(buf.is_empty());
}

TEST(mutation, consume_head_more_than_length) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 100);
  buf.append_tail(100);
  auto original_end = buf.end();
  ASSERT_EQ(1UL, buf.consume_head(101));
  ASSERT_EQ(0UL, buf.size());
  ASSERT_EQ(buf.begin(), buf.end());
  ASSERT_EQ(original_end, buf.end());
  ASSERT_TRUE(buf.is_empty());
}

TEST(mutation, consume_tail_more_than_length) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 100);
  buf.append_tail(100);
  auto original_begin = buf.begin();
  ASSERT_EQ(1UL, buf.consume_tail(101));
  ASSERT_EQ(0UL, buf.size());
  ASSERT_EQ(buf.begin(), buf.end());
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_TRUE(buf.is_empty());
}

TEST(mutation, consume_head_exact_length) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 100);
  buf.append_tail(100);
  auto original_end = buf.end();
  ASSERT_EQ(0UL, buf.consume_head(100));
  ASSERT_EQ(0UL, buf.size());
  ASSERT_EQ(buf.begin(), buf.end());
  ASSERT_EQ(original_end, buf.end());
  ASSERT_TRUE(buf.is_empty());
}

TEST(mutation, consume_tail_exact_length) {
  auto buf = buffer(make< trackable_heap_buf >(1024), 0, 100);
  buf.append_tail(100);
  auto original_begin = buf.begin();
  ASSERT_EQ(0UL, buf.consume_tail(100));
  ASSERT_EQ(0UL, buf.size());
  ASSERT_EQ(buf.begin(), buf.end());
  ASSERT_EQ(original_begin, buf.begin());
  ASSERT_TRUE(buf.is_empty());
}
