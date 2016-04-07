#include "xi/io/byte_blob.h"

#include <gtest/gtest.h>

using namespace xi;
using namespace xi::io;

byte_blob
make_blob(usize size) {
  auto mem = reinterpret_cast< u8* >(::malloc(size));
  XI_SCOPE(failure) {
    ::free(mem);
  };
  auto b = byte_blob(mem, mem ? size : 0, io::detail::make_simple_guard(mem));
  if (!b.empty()) {
    int i = 0;
    std::generate_n(begin(b), size, [&] { return ++i; });
  }
  return b;
}

byte_blob
make_blob(string data) {
  auto mem = reinterpret_cast< u8* >(::malloc(data.size()));
  XI_SCOPE(failure) {
    ::free(mem);
  };
  auto b =
      byte_blob(mem, mem ? data.size() : 0, io::detail::make_simple_guard(mem));
  if (!b.empty()) {
    std::copy(begin(data), end(data), begin(b));
  }
  return b;
}

TEST(interface, empty) {
  byte_blob b;
  EXPECT_TRUE(b.empty());
  EXPECT_EQ(nullptr, b.begin());
  EXPECT_EQ(nullptr, b.end());
  EXPECT_EQ(0u, b.size());

  byte_blob c{move(b)};
  EXPECT_TRUE(c.empty());
  EXPECT_EQ(nullptr, c.begin());
  EXPECT_EQ(nullptr, c.end());
  EXPECT_EQ(0u, c.size());

  byte_blob d = move(c);
  EXPECT_TRUE(d.empty());
  EXPECT_EQ(nullptr, d.begin());
  EXPECT_EQ(nullptr, d.end());
  EXPECT_EQ(0u, d.size());
}

TEST(interface, non_empty) {
  byte_blob b = make_blob("ABCD");
  EXPECT_FALSE(b.empty());
  EXPECT_NE(nullptr, b.begin());
  EXPECT_NE(nullptr, b.end());
  EXPECT_EQ(4u, b.size());
  EXPECT_EQ(0, memcmp("ABCD", b.data(), 4));

  byte_blob c{move(b)};
  EXPECT_FALSE(c.empty());
  EXPECT_NE(nullptr, c.begin());
  EXPECT_NE(nullptr, c.end());
  EXPECT_EQ(4u, c.size());
  EXPECT_EQ(0, memcmp("ABCD", c.data(), 4));

  byte_blob d = move(c);
  EXPECT_FALSE(d.empty());
  EXPECT_NE(nullptr, d.begin());
  EXPECT_NE(nullptr, d.end());
  EXPECT_EQ(4u, d.size());
  EXPECT_EQ(0, memcmp("ABCD", d.data(), 4));
}

TEST(clone, empty) {
  byte_blob b;
  auto c = b.clone();
  EXPECT_TRUE(c.empty());
  EXPECT_EQ(nullptr, c.begin());
  EXPECT_EQ(nullptr, c.end());
  EXPECT_EQ(0u, c.size());
}

TEST(clone, non_empty) {
  byte_blob b = make_blob("ABCD");
  auto c      = b.clone();
  EXPECT_FALSE(b.empty());
  EXPECT_NE(nullptr, b.begin());
  EXPECT_NE(nullptr, b.end());
  EXPECT_EQ(4u, b.size());
  EXPECT_EQ(0, memcmp("ABCD", b.data(), 4));
  EXPECT_EQ(b.data(), c.data());
  EXPECT_EQ(b.begin(), c.begin());
  EXPECT_EQ(b.end(), c.end());
}

TEST(split, empty) {
  byte_blob b;
  auto c = b.split(0);
  EXPECT_TRUE(c.empty());
  EXPECT_EQ(nullptr, c.begin());
  EXPECT_EQ(nullptr, c.end());
  EXPECT_EQ(0u, c.size());

  auto d = b.split(1 << 10);
  EXPECT_TRUE(d.empty());
  EXPECT_EQ(nullptr, d.begin());
  EXPECT_EQ(nullptr, d.end());
  EXPECT_EQ(0u, d.size());
}

TEST(split, non_empty_at_0) {
  byte_blob b = make_blob("ABCD");
  auto c      = b.split(0);
  EXPECT_TRUE(b.empty());
  EXPECT_EQ(nullptr, b.begin());
  EXPECT_EQ(nullptr, b.end());
  EXPECT_EQ(0u, b.size());

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(0, memcmp("ABCD", c.data(), 4));
  EXPECT_NE(nullptr, c.begin());
  EXPECT_NE(nullptr, c.end());
  EXPECT_EQ(4u, c.size());
}

TEST(split, non_empty_at_2) {
  byte_blob b = make_blob("ABCD");
  auto c      = b.split(2);
  EXPECT_FALSE(b.empty());
  EXPECT_EQ(0, memcmp("AB", b.data(), 2));
  EXPECT_NE(nullptr, b.begin());
  EXPECT_NE(nullptr, b.end());
  EXPECT_EQ(2u, b.size());

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(0, memcmp("CD", c.data(), 2));
  EXPECT_NE(nullptr, c.begin());
  EXPECT_NE(nullptr, c.end());
  EXPECT_EQ(2u, c.size());

  EXPECT_EQ(b.begin() + 2, c.begin());
  EXPECT_EQ(b.end() + 2, c.end());
}

TEST(split, non_empty_at_100) {
  byte_blob b = make_blob("ABCD");
  auto c      = b.split(100);
  EXPECT_TRUE(c.empty());
  EXPECT_EQ(nullptr, c.begin());
  EXPECT_EQ(nullptr, c.end());
  EXPECT_EQ(0u, c.size());

  EXPECT_FALSE(b.empty());
  EXPECT_EQ(0, memcmp("ABCD", b.data(), 4));
  EXPECT_NE(nullptr, b.begin());
  EXPECT_NE(nullptr, b.end());
  EXPECT_EQ(4u, b.size());
}

TEST(share, empty) {
  byte_blob b;
  auto c = b.share(0, 0);
  EXPECT_TRUE(c.empty());
  EXPECT_EQ(nullptr, c.begin());
  EXPECT_EQ(nullptr, c.end());
  EXPECT_EQ(0u, c.size());

  auto d = b.share(0, 1 << 10);
  EXPECT_TRUE(d.empty());
  EXPECT_EQ(nullptr, d.begin());
  EXPECT_EQ(nullptr, d.end());
  EXPECT_EQ(0u, d.size());
}

TEST(share, non_empty_at_0) {
  byte_blob b = make_blob("ABCD");
  auto c      = b.share(0);
  EXPECT_FALSE(b.empty());
  EXPECT_EQ(0, memcmp("ABCD", b.data(), 4));
  EXPECT_NE(nullptr, b.begin());
  EXPECT_NE(nullptr, b.end());
  EXPECT_EQ(4u, b.size());

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(0, memcmp("ABCD", c.data(), 4));
  EXPECT_NE(nullptr, c.begin());
  EXPECT_NE(nullptr, c.end());
  EXPECT_EQ(4u, c.size());

  EXPECT_EQ(b.begin(), c.begin());
  EXPECT_EQ(b.end(), c.end());
}

TEST(share, non_empty_at_1_1) {
  byte_blob b = make_blob("ABCD");
  auto c      = b.share(1, 1);
  EXPECT_FALSE(b.empty());
  EXPECT_EQ(0, memcmp("ABCD", b.data(), 4));
  EXPECT_NE(nullptr, b.begin());
  EXPECT_NE(nullptr, b.end());
  EXPECT_EQ(4u, b.size());

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(0, memcmp("B", c.data(), 1));
  EXPECT_NE(nullptr, c.begin());
  EXPECT_NE(nullptr, c.end());
  EXPECT_EQ(1u, c.size());

  EXPECT_EQ(b.begin() + 1, c.begin());
  EXPECT_EQ(b.end() - 2, c.end());
}

TEST(share, non_empty_at_1) {
  byte_blob b = make_blob("ABCD");
  auto c      = b.share(1);
  EXPECT_FALSE(b.empty());
  EXPECT_EQ(0, memcmp("ABCD", b.data(), 4));
  EXPECT_NE(nullptr, b.begin());
  EXPECT_NE(nullptr, b.end());
  EXPECT_EQ(4u, b.size());

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(0, memcmp("BCD", c.data(), 3));
  EXPECT_NE(nullptr, c.begin());
  EXPECT_NE(nullptr, c.end());
  EXPECT_EQ(3u, c.size());

  EXPECT_EQ(b.begin() + 1, c.begin());
  EXPECT_EQ(b.end(), c.end());
}

TEST(share, non_empty_at_100) {
  byte_blob b = make_blob("ABCD");
  auto c      = b.share(100);
  EXPECT_FALSE(b.empty());
  EXPECT_EQ(0, memcmp("ABCD", b.data(), 4));
  EXPECT_NE(nullptr, b.begin());
  EXPECT_NE(nullptr, b.end());
  EXPECT_EQ(4u, b.size());

  EXPECT_TRUE(c.empty());
  EXPECT_EQ(nullptr, c.begin());
  EXPECT_EQ(nullptr, c.end());
  EXPECT_EQ(0u, c.size());

  EXPECT_NE(b.begin(), c.begin());
  EXPECT_NE(b.end(), c.end());
}
