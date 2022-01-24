/**
 * @file event_manager.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "event_manager.h"

#include <utility>

using namespace taotu;

EventManager::EventManager() {}
EventManager::~EventManager() {
  if (thread_->joinable()) {
    thread_->join();
  }
}

void EventManager::Loop() {
  // TODO:
}

void EventManager::RunAt(TimePoint time_point, Timer::TimeCallback TimeTask) {
  timer_.AddTimeTask(std::move(time_point), std::move(TimeTask));
}
void EventManager::RunAfter(int64_t delay_seconds,
                            Timer::TimeCallback TimeTask) {
  timer_.AddTimeTask(TimePoint{delay_seconds}, std::move(TimeTask));
}
void EventManager::RunEveryUntil(int64_t interval_seconds,
                                 Timer::TimeCallback TimeTask,
                                 std::function<bool()> IsContinue) {
  TimePoint time_point{interval_seconds, true};
  if (IsContinue) {
    time_point.SetTaskContinueCallback(std::move(IsContinue));
  }
  timer_.AddTimeTask(std::move(time_point), std::move(TimeTask));
}

// void EventManager::UpdateEventer(Eventer *eventer) {}
// void EventManager::RemoveEventer(Eventer *eventer) {}

void EventManager::DoExpiredTimeTasks() {
  Timer::ExpiredTimeTasks expired_time_tasks = timer_.GetExpiredTimeTasks();
  for (auto &expired_time_task : expired_time_tasks) {
    auto ExpiredTimeCallback = expired_time_task.second;
    if (ExpiredTimeCallback) {
      ExpiredTimeCallback();
    }
    auto context = expired_time_task.first.GetContext();
    if (0 != context) {
      auto IsContinue = expired_time_task.first.GetTaskContinueCallback();
      if (IsContinue) {
        if (IsContinue()) {
          RunEveryUntil(context, std::move(ExpiredTimeCallback),
                        std::move(IsContinue));
        }
      } else {
        RunEveryUntil(context, std::move(ExpiredTimeCallback));
      }
    }
  }
}
