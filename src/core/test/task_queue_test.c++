#include <gtest/gtest.h>

#include "xi/core/task_queue.h"
#include "util.h"

using namespace xi;
using xi::core::task_queue;

TEST(simple, add_task_and_process) {
  task_queue _queue(1000);
  int i = 0;
  _queue.submit([&] { i = 42; });
  _queue.process_tasks();
  ASSERT_EQ(42, i);
}

TEST(simple, add_task_and_process_f_i_f_o) {
  task_queue _queue(1000);
  int i = 0;
  _queue.submit([&] { i += 42; });
  _queue.submit([&] { i /= 2; });
  _queue.process_tasks();
  ASSERT_EQ(21, i);
}

struct destructor_tracker_task : public test::object_tracker,
                                 public xi::core::task {
  static size_t RUN;
  void run() override { RUN++; }
};

size_t destructor_tracker_task::RUN = 0;

TEST(simple, tasks_destroyed_after_run) {
  task_queue _queue(1000);
  _queue.submit(destructor_tracker_task());
  ASSERT_EQ(1UL, destructor_tracker_task::CREATED);
  ASSERT_EQ(1UL, destructor_tracker_task::DESTROYED); // the copy
  ASSERT_EQ(0UL, destructor_tracker_task::RUN);

  _queue.process_tasks();
  ASSERT_EQ(1UL, destructor_tracker_task::CREATED);
  ASSERT_EQ(2UL, destructor_tracker_task::DESTROYED);
  ASSERT_EQ(1UL, destructor_tracker_task::RUN);
}

struct big_task : public xi::core::task {
  static size_t RUN;
  void run() override { RUN++; }

  char payload[1024];
};
size_t big_task::RUN = 0;

TEST(overflow, tasks_beyond_capacity_are_queued) {
  task_queue _queue(1000);
  _queue.submit(big_task());
  ASSERT_EQ(0UL, big_task::RUN);
  ASSERT_LT(1000UL, sizeof(big_task));

  _queue.process_tasks();
  ASSERT_EQ(1UL, big_task::RUN);
}
