#include <gtest/gtest.h>

#include "xi/core/TaskQueue.h"
#include "Util.h"

using namespace xi;
using xi::core::TaskQueue;

TEST(Simple, AddTaskAndProcess) {
  TaskQueue _queue(1000);
  int i = 0;
  _queue.submit([&] { i = 42; });
  _queue.processTasks();
  ASSERT_EQ(42, i);
}

TEST(Simple, AddTaskAndProcessFIFO) {
  TaskQueue _queue(1000);
  int i = 0;
  _queue.submit([&] { i += 42; });
  _queue.submit([&] { i /= 2; });
  _queue.processTasks();
  ASSERT_EQ(21, i);
}

struct DestructorTrackerTask : public test::ObjectTracker, public xi::core::Task {
  static size_t RUN;
  void run() override { RUN++; }
};

size_t DestructorTrackerTask::RUN = 0;

TEST(Simple, TasksDestroyedAfterRun) {
  TaskQueue _queue(1000);
  _queue.submit(DestructorTrackerTask());
  ASSERT_EQ(1UL, DestructorTrackerTask::CREATED);
  ASSERT_EQ(1UL, DestructorTrackerTask::DESTROYED); // the copy
  ASSERT_EQ(0UL, DestructorTrackerTask::RUN);

  _queue.processTasks();
  ASSERT_EQ(1UL, DestructorTrackerTask::CREATED);
  ASSERT_EQ(2UL, DestructorTrackerTask::DESTROYED);
  ASSERT_EQ(1UL, DestructorTrackerTask::RUN);
}

struct BigTask : public xi::core::Task {
  static size_t RUN;
  void run() override { RUN++; }

  char payload[1024];
};
size_t BigTask::RUN = 0;

TEST(Overflow, TasksBeyondCapacityAreQueued){
  TaskQueue _queue(1000);
  _queue.submit(BigTask());
  ASSERT_EQ(0UL, BigTask::RUN);
  ASSERT_LT(1000UL, sizeof(BigTask));

  _queue.processTasks();
  ASSERT_EQ(1UL, BigTask::RUN);
}
