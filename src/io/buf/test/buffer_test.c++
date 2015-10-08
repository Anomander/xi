// #include "xi/io/buf/buffer.h"
// #include "xi/io/buf/heap_buf.h"

// #include <gtest/gtest.h>

// #include "src/test/util.h"

// using namespace xi;
// using namespace xi::io;

// struct trackable_heap_buf : public heap_buf, public test::object_tracker {
//   using heap_buf::heap_buf;
// };

// TEST(assertions, buffer_larger_than_arena) {
//   ASSERT_THROW(buffer::create(make< trackable_heap_buf >(10), 0, 11),
//                std::out_of_range);
//   ASSERT_THROW(buffer::create(make< trackable_heap_buf >(10), 1, 10),
//                std::out_of_range);
//   ASSERT_THROW(buffer::create(make< trackable_heap_buf >(10), 9, -7),
//                std::out_of_range);
//   ASSERT_THROW(buffer::create(make< trackable_heap_buf >(10), -6, -7),
//                std::out_of_range);
//   ASSERT_THROW(buffer::create(make< trackable_heap_buf >(10), 10, 1),
//                std::out_of_range);
//   ASSERT_THROW(buffer::create(make< trackable_heap_buf >(10), 11, -1),
//                std::out_of_range);
// }

// TEST(assertions, buffer_fits_in_arena) {
//   ASSERT_NO_THROW(buffer::create(make< trackable_heap_buf >(10), 0, 10));
//   ASSERT_NO_THROW(buffer::create(make< trackable_heap_buf >(10), 1, 9));
//   ASSERT_NO_THROW(buffer::create(make< trackable_heap_buf >(10), 9, 1));
//   ASSERT_NO_THROW(buffer::create(make< trackable_heap_buf >(10), 10, 0));
// }

// TEST(interface, begin_equals_writable_bytes) {
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   ASSERT_EQ(buf->writable_bytes(), buf->begin());
// }

// TEST(interface, end_equals_writable_bytes_plus_length) {
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   ASSERT_EQ(buf->writable_bytes() + buf->length(), buf->end());
// }

// TEST(interface, length_is_correct) {
//   ASSERT_EQ(100UL,
//             buffer::create(make< trackable_heap_buf >(1024), 0, 100)->length());
//   ASSERT_EQ(0UL,
//             buffer::create(make< trackable_heap_buf >(1024), 0, 0)->length());
//   ASSERT_EQ(1024UL, buffer::create(make< trackable_heap_buf >(1024), 0, 1024)
//                         ->length());
// }

// TEST(interface, is_empty_is_correct) {
//   ASSERT_FALSE(
//       buffer::create(make< trackable_heap_buf >(1024), 0, 100)->is_empty());
//   ASSERT_TRUE(
//       buffer::create(make< trackable_heap_buf >(1024), 0, 0)->is_empty());
// }

// TEST(lifetime, arena_is_destroyed_with_buffer) {
//   trackable_heap_buf::reset();
//   {
//     auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//     ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
//     ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);
//   }
//   ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
//   ASSERT_EQ(1UL, trackable_heap_buf::DESTROYED);
// }

// TEST(lifetime, view_prolongs_arena_lifetime) {
//   trackable_heap_buf::reset();
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   {
//     auto view = buf->make_view();
//     release(move(buf));
//     ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
//     ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);
//   }
//   ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
//   ASSERT_EQ(1UL, trackable_heap_buf::DESTROYED);
// }

// TEST(lifetime, view_copies_prolong_arena_lifetime) {
//   trackable_heap_buf::reset();
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   {
//     auto view = make_unique_copy(buf->make_view());
//     {
//       release(move(buf));
//       ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
//       ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);
//     }
//     auto view_copy = buffer::view(*view);
//     ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
//     ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);

//     release(move(view));
//     ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
//     ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);
//   }
//   ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
//   ASSERT_EQ(1UL, trackable_heap_buf::DESTROYED);
// }

// TEST(lifetime_chain, head_owns_chain_items) {
//   trackable_heap_buf::reset();
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   ASSERT_EQ(1UL, trackable_heap_buf::CREATED);
//   ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);
//   buf->push_back(buffer::create(make< trackable_heap_buf >(1024), 0, 100));
//   ASSERT_EQ(2UL, trackable_heap_buf::CREATED);
//   ASSERT_EQ(0UL, trackable_heap_buf::DESTROYED);
//   release(move(buf));
//   ASSERT_EQ(2UL, trackable_heap_buf::CREATED);
//   ASSERT_EQ(2UL, trackable_heap_buf::DESTROYED);
// }

// TEST(mutation, consume_head_less_than_length) {
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   auto original_begin = buf->begin();
//   auto original_end = buf->end();
//   ASSERT_EQ(0UL, buf->consume_head(1));
//   ASSERT_EQ(99UL, buf->length());
//   ASSERT_EQ(original_begin + 1, buf->begin());
//   ASSERT_EQ(original_end, buf->end());

//   ASSERT_EQ(0UL, buf->consume_head(10));
//   ASSERT_EQ(89UL, buf->length());
//   ASSERT_EQ(original_begin + 11, buf->begin());
//   ASSERT_EQ(original_end, buf->end());

//   ASSERT_EQ(0UL, buf->consume_head(50));
//   ASSERT_EQ(39UL, buf->length());
//   ASSERT_EQ(original_begin + 61, buf->begin());
//   ASSERT_EQ(original_end, buf->end());

//   ASSERT_EQ(0UL, buf->consume_head(39));
//   ASSERT_EQ(0UL, buf->length());
//   ASSERT_EQ(original_begin + 100, buf->begin());
//   ASSERT_EQ(original_end, buf->end());
//   ASSERT_EQ(buf->begin(), buf->end());
//   ASSERT_TRUE(buf->is_empty());
// }

// TEST(mutation, consume_tail_less_than_length) {
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   auto original_begin = buf->begin();
//   auto original_end = buf->end();
//   ASSERT_EQ(0UL, buf->consume_tail(1));
//   ASSERT_EQ(99UL, buf->length());
//   ASSERT_EQ(original_end - 1, buf->end());
//   ASSERT_EQ(original_begin, buf->begin());

//   ASSERT_EQ(0UL, buf->consume_tail(10));
//   ASSERT_EQ(89UL, buf->length());
//   ASSERT_EQ(original_end - 11, buf->end());
//   ASSERT_EQ(original_begin, buf->begin());

//   ASSERT_EQ(0UL, buf->consume_tail(50));
//   ASSERT_EQ(39UL, buf->length());
//   ASSERT_EQ(original_end - 61, buf->end());
//   ASSERT_EQ(original_begin, buf->begin());

//   ASSERT_EQ(0UL, buf->consume_tail(39));
//   ASSERT_EQ(0UL, buf->length());
//   ASSERT_EQ(original_end - 100, buf->end());
//   ASSERT_EQ(original_begin, buf->begin());
//   ASSERT_EQ(buf->begin(), buf->end());
//   ASSERT_TRUE(buf->is_empty());
// }

// TEST(mutation, consume_head_more_than_length) {
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   auto original_end = buf->end();
//   ASSERT_EQ(1UL, buf->consume_head(101));
//   ASSERT_EQ(0UL, buf->length());
//   ASSERT_EQ(buf->begin(), buf->end());
//   ASSERT_EQ(original_end, buf->end());
//   ASSERT_TRUE(buf->is_empty());
// }

// TEST(mutation, consume_tail_more_than_length) {
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   auto original_begin = buf->begin();
//   ASSERT_EQ(1UL, buf->consume_tail(101));
//   ASSERT_EQ(0UL, buf->length());
//   ASSERT_EQ(buf->begin(), buf->end());
//   ASSERT_EQ(original_begin, buf->begin());
//   ASSERT_TRUE(buf->is_empty());
// }

// TEST(mutation, consume_head_exact_length) {
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   auto original_end = buf->end();
//   ASSERT_EQ(0UL, buf->consume_head(100));
//   ASSERT_EQ(0UL, buf->length());
//   ASSERT_EQ(buf->begin(), buf->end());
//   ASSERT_EQ(original_end, buf->end());
//   ASSERT_TRUE(buf->is_empty());
// }

// TEST(mutation, consume_tail_exact_length) {
//   auto buf = buffer::create(make< trackable_heap_buf >(1024), 0, 100);
//   auto original_begin = buf->begin();
//   ASSERT_EQ(0UL, buf->consume_tail(100));
//   ASSERT_EQ(0UL, buf->length());
//   ASSERT_EQ(buf->begin(), buf->end());
//   ASSERT_EQ(original_begin, buf->begin());
//   ASSERT_TRUE(buf->is_empty());
// }
