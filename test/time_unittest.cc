#include <gtest/gtest.h>
#include <stdio.h>
#include <unistd.h>

#include "../src/time_point.h"
#include "../src/timer.h"

TEST(TimeTest, TimePointTest) {
  taotu::TimePoint time_point1;
  taotu::TimePoint time_point2{4 * 1000 * 1000};
  taotu::TimePoint time_point3{3 * 1000 * 1000};
  taotu::TimePoint time_point4 = time_point3;
  ASSERT_TRUE(time_point1 < time_point2);
  ASSERT_FALSE(time_point2 <= time_point3);
  ASSERT_TRUE(time_point3 == time_point4);
  ASSERT_TRUE(time_point3 <= time_point4);
  auto res1 = static_cast<int>(time_point2.GetMicroseconds() / 1000);
  auto res2 = static_cast<int>(time_point2.GetMillisecond());
  ASSERT_EQ(res1, res2);
}

TEST(TimeTest, TimerTest) {
  taotu::Timer timer;
  ASSERT_EQ(timer.GetMinTimeDuration(), 10000);
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
  ASSERT_EQ(timer.GetMinTimeDuration(), 0);
  ::sleep(4);
  auto time_task_pair_list = timer.GetExpiredTimeTasks();
  ASSERT_EQ(time_task_pair_list[0].first.GetMillisecond(),
            time_point3.GetMillisecond());
  time_task_pair_list[0].second();
  ASSERT_EQ(flag3, 3);
  ASSERT_EQ(time_task_pair_list[1].first.GetMillisecond(),
            time_point2.GetMillisecond());
  time_task_pair_list[1].second();
  ASSERT_EQ(flag2, 2);
  ASSERT_EQ(time_task_pair_list[2].first.GetMillisecond(),
            time_point1.GetMillisecond());
  time_task_pair_list[2].second();
  ASSERT_EQ(flag1, 1);
  ASSERT_EQ(timer.GetMinTimeDuration(), 10000);
}
