#include <gtest/gtest.h>

#include "xi/io/stream_buffer.h"

using namespace xi::io;

uint8_t DATA[] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
    34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
};

void assert_arrays_eq(uint8_t *lhs, uint8_t *rhs, size_t len) {
  for (size_t i = 0; i < len; ++i) { ASSERT_EQ(*(lhs + i), *(rhs + i)); }
}

TEST(simple, push) {
  byte_range range{DATA, 10};
  stream_buffer buffer;
  ASSERT_TRUE(buffer.empty());
  buffer.push(range);

  ASSERT_FALSE(buffer.empty());
  byte_range vec[256];
  ASSERT_EQ(1UL, buffer.fill_vec(vec, 256));
  assert_arrays_eq(DATA, (uint8_t *)vec[0].data, 10);
  ASSERT_EQ(10UL, vec[0].size);
}

TEST(simple, no_consume_returns_same_data) {
  byte_range range{DATA, 10};
  stream_buffer buffer;
  ASSERT_TRUE(buffer.empty());
  buffer.push(range);

  ASSERT_FALSE(buffer.empty());
  byte_range vec[256];
  ASSERT_EQ(1UL, buffer.fill_vec(vec, 256));
  assert_arrays_eq(DATA, (uint8_t *)vec[0].data, 10);
  ASSERT_EQ(10UL, vec[0].size);

  ASSERT_FALSE(buffer.empty());
  ASSERT_EQ(1UL, buffer.fill_vec(vec, 256));
  assert_arrays_eq(DATA, (uint8_t *)vec[0].data, 10);
  ASSERT_EQ(10UL, vec[0].size);
}

TEST(simple, consume_offsets_returned_data) {
  byte_range range{DATA, 10};
  stream_buffer buffer;
  ASSERT_TRUE(buffer.empty());
  buffer.push(range);

  ASSERT_FALSE(buffer.empty());
  byte_range vec[256];
  ASSERT_EQ(1UL, buffer.fill_vec(vec, 256));
  assert_arrays_eq(DATA, (uint8_t *)vec[0].data, 10);
  ASSERT_EQ(10UL, vec[0].size);

  for (size_t i = 9; i > 0; --i) {
    buffer.consume(1);
    ASSERT_EQ(1UL, buffer.fill_vec(vec, 256));
    assert_arrays_eq(DATA + (10 - i), (uint8_t *)vec[0].data, i);
    ASSERT_EQ(i, vec[0].size);
  }
  buffer.consume(1);
  ASSERT_EQ(0UL, buffer.fill_vec(vec, 256));
  ASSERT_TRUE(buffer.empty());
}

TEST(simple, consume_more_than_has_is_fine) {
  byte_range range{DATA, 10};
  stream_buffer buffer;
  ASSERT_TRUE(buffer.empty());
  buffer.push(range);

  ASSERT_FALSE(buffer.empty());
  buffer.consume(256);
  ASSERT_TRUE(buffer.empty());
}
