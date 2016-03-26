#include <gtest/gtest.h>

#include "xi/core/latch.h"

using namespace xi::core;

TEST(simple, count_one_Need_one) {
  latch l(1);

  bool b = false;
  l.await().then([&b] { b = true; });

  ASSERT_FALSE(b);

  l.count_down();

  ASSERT_TRUE(b);
}

TEST(simple, count_all_One_go) {
  latch l(10);

  bool b = false;
  l.await().then([&b] { b = true; });

  ASSERT_FALSE(b);

  l.count_down(10);

  ASSERT_TRUE(b);
}

TEST(simple, count_all_One_by_one) {
  latch l(5);

  bool b = false;
  l.await().then([&b] { b = true; });

  ASSERT_FALSE(b);
  l.count_down();
  ASSERT_FALSE(b);
  l.count_down();
  ASSERT_FALSE(b);
  l.count_down();
  ASSERT_FALSE(b);
  l.count_down();
  ASSERT_FALSE(b);
  l.count_down();

  ASSERT_TRUE(b);
}

TEST(simple, count_more_than_available) {
  latch l(5);

  bool b = false;
  l.await().then([&b] { b = true; });

  ASSERT_FALSE(b);
  l.count_down(10);

  ASSERT_TRUE(b);
}

TEST(simple, await_after_count_all) {
  latch l(1);

  bool b = false;

  l.count_down();

  l.await().then([&b] { b = true; });

  ASSERT_TRUE(b);
}

TEST(simple, await_in_the_middle) {
  latch l(2);

  bool b = false;

  l.count_down();

  l.await().then([&b] { b = true; });
  ASSERT_FALSE(b);

  l.count_down();
  ASSERT_TRUE(b);
}

TEST(failure, zero_capacity_starts_ready) {
  latch l(0);

  bool b = false;

  l.await().then([&b] { b = true; });
  ASSERT_TRUE(b);
}
