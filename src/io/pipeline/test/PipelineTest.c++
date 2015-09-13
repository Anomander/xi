#include "xi/io/pipeline/Pipeline.h"
#include "xi/io/pipeline/Util.h"

using namespace xi;
using namespace xi::io::pipeline;

#include <gtest/gtest.h>

TEST(Simple, DownstreamEventCorrectOrder) {
  Pipeline p{nullptr};
  int i = 0;

  p.pushBack(makeHandler< MessageRead >([&](auto cx, auto e) {
    i += 2;
    cx->forward(move(e));
  }));
  p.pushBack(makeHandler< MessageRead >([&](auto cx, auto e) { i *= 2; }));

  p.fire(MessageRead{nullptr});

  ASSERT_EQ(4, i);
}

TEST(Simple, UpstreamEventCorrectOrder) {
  Pipeline p{nullptr};
  int i = 0;

  p.pushBack(makeHandler< MessageWrite >([&](auto cx, auto e) { i *= 2; }));
  p.pushBack(makeHandler< MessageWrite >([&](auto cx, auto e) {
    i += 2;
    cx->forward(move(e));
  }));

  p.fire(MessageWrite{nullptr});

  ASSERT_EQ(4, i);
}

TEST(Simple, ExceptionInHandlerPropagatesOut) {
  Pipeline p{nullptr};
  int i = 0;
  bool exceptionHandled = false;

  p.pushBack(makeHandler< MessageWrite >([&](auto cx, auto e) { FAIL(); }));
  p.pushBack(makeHandler< MessageWrite >([&](auto cx, auto e) { throw std::exception(); }));
  p.pushBack(makeHandler< MessageWrite >([&](auto cx, auto e) {
    i += 2;
    cx->forward(move(e));
  }));
  p.pushBack(makeHandler< ChannelException >([&](auto cx, auto e) { exceptionHandled = true; }));

  ASSERT_NO_THROW(p.fire(MessageWrite{nullptr}));

  ASSERT_EQ(2, i);
  ASSERT_TRUE(exceptionHandled);
}

TEST(Simple, NoForwardInHandlerStopsPipeline) {
  Pipeline p{nullptr};
  int i = 0;

  p.pushBack(makeHandler< MessageWrite >([&](auto cx, auto e) { i *= 100; }));
  p.pushBack(makeHandler< MessageWrite >([&](auto cx, auto e) { i += 2; }));

  ASSERT_NO_THROW(p.fire(MessageWrite{nullptr}));

  ASSERT_EQ(2, i);
}
