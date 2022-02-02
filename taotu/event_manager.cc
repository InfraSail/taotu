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

#include <pthread.h>

#include <memory>
#include <string>
#include <utility>

#include "logger.h"
#include "spin_lock.h"
#include "timer.h"

using namespace taotu;

EventManager::EventManager() : eventer_amount_(0) {
  poller_ = std::make_unique<Poller>(&eventer_amount_);
}
EventManager::~EventManager() {
  if (thread_->joinable()) {
    thread_->join();
  }
}

void EventManager::Loop() {
  thread_ = std::make_unique<std::thread>([this]() {
    should_quit_ = false;
    LOG(logger::kDebug, "The event loop in thread(" +
                            std::to_string(::pthread_self()) +
                            ") is starting.");
    while (!should_quit_) {
      DoWithActiveTasks(
          poller_->Poll(timer_.GetMinTimeDurationSet(), &active_events_));
      DoExpiredTimeTasks();
      DestroyClosedConnections();
    }
    LOG(logger::kDebug, "The event loop in thread(" +
                            std::to_string(::pthread_self()) +
                            ") is stopping.");
  });
}

void EventManager::InsertNewConnection(int socket_fd,
                                       const NetAddress& local_address,
                                       const NetAddress& peer_address) {
  LockGuard lock_guard(eventer_map_mutex_lock_);
  eventer_map_[socket_fd] = std::make_unique<Connecting>(
      this, socket_fd, local_address, peer_address);
  ++eventer_amount_;
}

void EventManager::RunAt(TimePoint time_point, Timer::TimeCallback TimeTask) {
  timer_.AddTimeTask(std::move(time_point), std::move(TimeTask));
}
void EventManager::RunAfter(int64_t delay_microseconds,
                            Timer::TimeCallback TimeTask) {
  timer_.AddTimeTask(TimePoint{delay_microseconds}, std::move(TimeTask));
}
void EventManager::RunEveryUntil(int64_t interval_microseconds,
                                 Timer::TimeCallback TimeTask,
                                 std::function<bool()> IsContinue) {
  TimePoint time_point{interval_microseconds, true};
  // Check if the function which decides whether to continue the cycle should be
  // set (for repeatable condition)
  if (IsContinue) {
    time_point.SetTaskContinueCallback(std::move(IsContinue));
  }
  timer_.AddTimeTask(std::move(time_point), std::move(TimeTask));
}

void EventManager::DoWithActiveTasks(TimePoint return_time) {
  for (auto active_event : active_events_) {
    active_event->Work(return_time);
    int fd = active_event->Fd();
    {
      LockGuard lock_guard(eventer_map_mutex_lock_);
      if (eventer_map_[fd]->IsClosed()) {
        --eventer_amount_;
        closed_fds.push_back(fd);
      }
    }
  }
  active_events_.clear();
}
void EventManager::DoExpiredTimeTasks() {
  Timer::ExpiredTimeTasks expired_time_tasks = timer_.GetExpiredTimeTasks();
  for (auto& expired_time_task : expired_time_tasks) {
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
void EventManager::DestroyClosedConnections() {
  for (auto fd : closed_fds) {
    {
      LockGuard lock_guard(eventer_map_mutex_lock_);
      eventer_map_.erase(fd);
    }
  }
  closed_fds.clear();
}
