#include <gtest/gtest.h>
#include <stdio.h>
#include <unistd.h>

#include "../src/time_point.h"
#include "../src/timer.h"

TEST(TimeTest, AddTimeTaskTest) {
  taotu::Timer timer;
  EXPECT_EQ(timer.GetMinTimeDurationSet(), 10000);
  int flag1 = 0;
  int flag2 = 0;
  int flag3 = 0;
  auto f1 = [&]() { flag1 = 1; };
  taotu::TimePoint time_point1{2 * 1000 * 1000};
  timer.AddTimeTask(time_point1, std::move(f1));
  auto f2 = [&]() { flag2 = 2; };
  taotu::TimePoint time_point2{1 * 1000 * 1000};
  timer.AddTimeTask(time_point2, std::move(f2));
  auto f3 = [&]() { flag3 = 3; };
  taotu::TimePoint time_point3;
  timer.AddTimeTask(time_point3, std::move(f3));
  EXPECT_EQ(timer.GetMinTimeDurationSet(), 0);
  ::sleep(4);
  auto time_task_pair_list = timer.GetExpiredTimeTasks();
  EXPECT_EQ(time_task_pair_list[0].first.GetMillisecond(),
            time_point3.GetMillisecond());
  time_task_pair_list[0].second();
  EXPECT_EQ(flag3, 3);
  EXPECT_EQ(time_task_pair_list[1].first.GetMillisecond(),
            time_point2.GetMillisecond());
  time_task_pair_list[1].second();
  EXPECT_EQ(flag2, 2);
  EXPECT_EQ(time_task_pair_list[2].first.GetMillisecond(),
            time_point1.GetMillisecond());
  time_task_pair_list[2].second();
  EXPECT_EQ(flag1, 1);
  EXPECT_EQ(timer.GetMinTimeDurationSet(), 10000);
}
