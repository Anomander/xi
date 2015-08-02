#include <gtest/gtest.h>

#include "async/TaskQueue.h"

using namespace xi;
using xi::async::TaskQueue;

TEST (Simple, AddTaskAndProcess) {
  TaskQueue _queue (1000);
  int i = 0;
  _queue.submit([&] { i = 42; });
  _queue.processTasks();
  ASSERT_EQ(42, i);
}

TEST (Simple, AddTaskAndProcessFIFO) {
  TaskQueue _queue (1000);
  int i = 0;
  _queue.submit([&] { i += 42; });
  _queue.submit([&] { i /= 2; });
  _queue.processTasks();
  ASSERT_EQ(21, i);
}
