/**
 * @file timer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-17
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "timer.h"

#include <utility>

using namespace taotu;

Timer::Timer(EventManager* event_manager) : event_manager_(event_manager) {}

void Timer::AddTimeTask(TimePoint time_point, TimeCallback time_callback) {
  time_points_.insert({time_point, std::move(time_callback)});
}

std::vector<Timer::TimeCallback> Timer::GetExpiredTimeTasks() {
  std::vector<TimeCallback> expired_time_tasks;
  return expired_time_tasks;
}
