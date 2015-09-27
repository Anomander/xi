#include "xi/io/pipeline/pipeline.h"
#include "xi/io/pipeline/util.h"

using namespace xi;
using namespace xi::io::pipeline;

#include <gtest/gtest.h>

TEST(simple, downstream_event_correct_order) {
  pipeline p{nullptr};
  int i = 0;

  p.push_back(make_handler< message_read >([&](auto cx, auto e) {
    i += 2;
    cx->forward(move(e));
  }));
  p.push_back(make_handler< message_read >([&](auto cx, auto e) { i *= 2; }));

  p.fire(message_read{nullptr});

  ASSERT_EQ(4, i);
}

TEST(simple, upstream_event_correct_order) {
  pipeline p{nullptr};
  int i = 0;

  p.push_back(make_handler< message_write >([&](auto cx, auto e) { i *= 2; }));
  p.push_back(make_handler< message_write >([&](auto cx, auto e) {
    i += 2;
    cx->forward(move(e));
  }));

  p.fire(message_write{nullptr});

  ASSERT_EQ(4, i);
}

TEST(simple, exception_in_handler_propagates_out) {
  pipeline p{nullptr};
  int i = 0;
  bool exception_handled = false;

  p.push_back(make_handler< message_write >([&](auto cx, auto e) { FAIL(); }));
  p.push_back(make_handler< message_write >(
      [&](auto cx, auto e) { throw std::exception(); }));
  p.push_back(make_handler< message_write >([&](auto cx, auto e) {
    i += 2;
    cx->forward(move(e));
  }));
  p.push_back(make_handler< channel_exception >(
      [&](auto cx, auto e) { exception_handled = true; }));

  ASSERT_NO_THROW(p.fire(message_write{nullptr}));

  ASSERT_EQ(2, i);
  ASSERT_TRUE(exception_handled);
}

TEST(simple, no_forward_in_handler_stops_pipeline) {
  pipeline p{nullptr};
  int i = 0;

  p.push_back(
      make_handler< message_write >([&](auto cx, auto e) { i *= 100; }));
  p.push_back(make_handler< message_write >([&](auto cx, auto e) { i += 2; }));

  ASSERT_NO_THROW(p.fire(message_write{nullptr}));

  ASSERT_EQ(2, i);
}
