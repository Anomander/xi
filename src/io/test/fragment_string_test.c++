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
make_string(initializer_list<usize> sizes) {
  vector< char > in(92);
  char i = 'A';
  std::generate_n(begin(in), 92, [&] { return i++; });

  intrusive::list< fragment,
                   intrusive::link_mode< intrusive::normal_link >,
                   intrusive::constant_time_size< true > > list;
  usize size = 0;
  for (auto sz : sizes) {
    auto arena = ALLOC->allocate_arena(sz);
    auto b     = new fragment(arena, arena->allocate(sz), sz);
    b->write(byte_range{in});
    list.push_back(*b);
    size += sz;
  }
  return fragment_string{move(list), size};
}

TEST(interface, empty_string) {
  fragment_string b;
  EXPECT_TRUE(b.empty());
  EXPECT_EQ(0u, b.length());
  EXPECT_EQ(0u, b.size());
}

TEST(interface, compare_empty) {
  fragment_string b;
  EXPECT_EQ(0, b.compare(""));
  EXPECT_TRUE(0 > b.compare("A"));
  EXPECT_TRUE(0 > b.compare("ABCD"));
}

TEST(interface, compare_single_fragment) {
  auto s = make_string({10});
  EXPECT_TRUE(0 < s.compare(""));
  EXPECT_TRUE(0 < s.compare("A"));
  EXPECT_TRUE(0 < s.compare("ABCD"));
  EXPECT_TRUE(0 > s.compare("a"));
  EXPECT_TRUE(0 > s.compare("abcd"));
  EXPECT_TRUE(0 == s.compare("ABCDEFGHIJ"));
  EXPECT_TRUE(0 > s.compare("ABCDEFGHIJK"));
  EXPECT_TRUE(0 > s.compare("abcdefghij"));
}

TEST(interface, compare_multi_fragment) {
  auto s = make_string({5,5});
  EXPECT_TRUE(0 < s.compare(""));
  EXPECT_TRUE(0 < s.compare("A"));
  EXPECT_TRUE(0 < s.compare("ABCD"));
  EXPECT_TRUE(0 > s.compare("a"));
  EXPECT_TRUE(0 > s.compare("abcd"));
  EXPECT_TRUE(0 == s.compare("ABCDEABCDE"));
  EXPECT_TRUE(0 > s.compare("ABCDEFABCDEFG"));
  EXPECT_TRUE(0 > s.compare("abcdeabcdef"));
}

TEST(interface, empty_copy_to_string) {
  fragment_string b;
  EXPECT_TRUE(b.copy_to_string().empty());
  EXPECT_TRUE(b.copy_to_string(1).empty());
  EXPECT_TRUE(b.copy_to_string(100).empty());
}

TEST(interface, copy_to_string_single_buffer) {
  auto s = make_string({10});
  EXPECT_EQ("ABCDEFGHIJ", s.copy_to_string());
  EXPECT_EQ("A", s.copy_to_string(1));
  EXPECT_EQ("ABCDEFGHIJ", s.copy_to_string(10));
  EXPECT_EQ("ABCDEFGHIJ", s.copy_to_string(100));
}

TEST(interface, copy_to_string_multi_buffer) {
  auto s = make_string({5,5});
  EXPECT_EQ("ABCDEABCDE", s.copy_to_string());
  EXPECT_EQ("A", s.copy_to_string(1));
  EXPECT_EQ("ABCDEA", s.copy_to_string(6));
  EXPECT_EQ("ABCDEABCDE", s.copy_to_string(10));
  EXPECT_EQ("ABCDEABCDE", s.copy_to_string(100));
}

TEST(interface, empty_compare_other) {
  fragment_string s;
  EXPECT_EQ(0, s.compare(fragment_string{}));
  EXPECT_TRUE(0 > s.compare(make_string({1})));
  EXPECT_TRUE(0 > s.compare(make_string({1,10})));
}

TEST(interface, compare_other_single_buffer) {
  auto s = make_string({10});
  EXPECT_TRUE(0 < s.compare(fragment_string{}));
  EXPECT_TRUE(0 < s.compare(make_string({1})));
  EXPECT_TRUE(0 < s.compare(make_string({1,1})));
  EXPECT_TRUE(0 == s.compare(make_string({10})));
  EXPECT_TRUE(0 > s.compare(make_string({11})));
}

TEST(interface, compare_other_multi_buffer) {
  auto s = make_string({5,5});
  EXPECT_TRUE(0 < s.compare(fragment_string{}));
  EXPECT_TRUE(0 < s.compare(make_string({1})));
  EXPECT_TRUE(0 < s.compare(make_string({1,1})));
  EXPECT_TRUE(0 == s.compare(make_string({5,5})));
  EXPECT_TRUE(0 > s.compare(make_string({11})));
}
