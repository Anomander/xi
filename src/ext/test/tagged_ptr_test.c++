#include "xi/ext/tagged_ptr.h"

#include <gtest/gtest.h>

using namespace xi;

TEST(interface, get_returns_correct_ptr) {
  auto b = new char[16];
  tagged_ptr< char, unsigned > p{b};

  ASSERT_EQ(b, p.get());
}

TEST(interface, object_fields_are_accessible) {
  struct foo {
    int i    = 42;
    double d = 42.42;
  } *b = new foo;
  tagged_ptr< foo, unsigned > p{b};

  ASSERT_EQ(42, p->i);
  ASSERT_EQ(42.42, p->d);
}

TEST(interface, set_tag_doesnt_change_pointer) {
  auto b = new char[16];
  tagged_ptr< char, unsigned > p{b};

  p.set_tag< 1 >();
  p.set_tag< 2 >();
  p.set_tag< 3 >();
  ASSERT_EQ(b, p.get());
}

TEST(interface, reset_clears_tags) {
  auto b = new char[16];
  tagged_ptr< char, unsigned > p{b};

  p.set_tag< 1 >();
  p.set_tag< 2 >();
  p.set_tag< 3 >();
  p.reset_tag< 1 >();
  p.reset_tag< 2 >();
  p.reset_tag< 3 >();
  ASSERT_EQ(0u, p.get_tag< 1 >());
  ASSERT_EQ(0u, p.get_tag< 2 >());
  ASSERT_EQ(0u, p.get_tag< 3 >());
}

TEST(interface, tags_which_are_set_can_be_got) {
  auto b = new char[16];
  tagged_ptr< char, unsigned > p{b};

  p.set_tag< 1 >();
  p.set_tag< 2 >();
  p.set_tag< 3 >();
  ASSERT_EQ(1u, p.get_tag< 1 >());
  ASSERT_EQ(2u, p.get_tag< 2 >());
  ASSERT_EQ(3u, p.get_tag< 3 >());
}

TEST(interface, tags_are_not_set_by_default) {
  auto b = new char[16];
  tagged_ptr< char, unsigned > p{b};

  ASSERT_EQ(0u, p.get_tag< 1 >());
  ASSERT_EQ(0u, p.get_tag< 2 >());
  ASSERT_EQ(0u, p.get_tag< 3 >());
}
