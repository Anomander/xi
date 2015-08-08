#include <gtest/gtest.h>

#include "async/Latch.h"

using namespace xi::async;

TEST(Simple, CountOne_NeedOne) {
  Latch l(1);

  bool b = false;
  l.await().then([&b] { b = true; });

  ASSERT_FALSE(b);

  l.countDown();

  ASSERT_TRUE(b);
}

TEST(Simple, CountAll_OneGo) {
  Latch l(10);

  bool b = false;
  l.await().then([&b] { b = true; });

  ASSERT_FALSE(b);

  l.countDown(10);

  ASSERT_TRUE(b);
}

TEST(Simple, CountAll_OneByOne) {
  Latch l(5);

  bool b = false;
  l.await().then([&b] { b = true; });

  ASSERT_FALSE(b);
  l.countDown();
  ASSERT_FALSE(b);
  l.countDown();
  ASSERT_FALSE(b);
  l.countDown();
  ASSERT_FALSE(b);
  l.countDown();
  ASSERT_FALSE(b);
  l.countDown();

  ASSERT_TRUE(b);
}

TEST(Simple, CountMoreThanAvailable) {
  Latch l(5);

  bool b = false;
  l.await().then([&b] { b = true; });

  ASSERT_FALSE(b);
  l.countDown(10);

  ASSERT_TRUE(b);
}

TEST(Simple, AwaitAfterCountAll) {
  Latch l(1);

  bool b = false;

  l.countDown();

  l.await().then([&b] { b = true; });

  ASSERT_TRUE(b);
}

TEST(Simple, AwaitInTheMiddle) {
  Latch l(2);

  bool b = false;

  l.countDown();

  l.await().then([&b] { b = true; });
  ASSERT_FALSE(b);

  l.countDown();
  ASSERT_TRUE(b);
}

TEST(Failure, ZeroCapacityStartsReady) {
  Latch l(0);

  bool b = false;

  l.await().then([&b] { b = true; });
  ASSERT_TRUE(b);
}
