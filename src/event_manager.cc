/**
 * @file event_manager.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "EventManager" (single "Reactor") which is the
 * manager of an event loop in single I/O thread.
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "event_manager.h"

// #include <pthread.h>

#include <string>
#include <utility>

#include "connecting.h"
#include "logger.h"
#include "spin_lock.h"
#include "timer.h"

using namespace taotu;

EventManager::EventManager() : poller_() {}
EventManager::~EventManager() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

void EventManager::Loop() {
  thread_ = std::thread([this] { this->Work(); });
}
void EventManager::Work() {
  should_quit_ = false;
  // LOG(logger::kDebug, "The event loop in thread(%lu) is starting.",
  //     ::pthread_self());
  while (!should_quit_) {
    auto return_time =
        poller_.Poll(timer_.GetMinTimeDurationSet(),
                     &active_events_);  // Return time is the time point of the
                                        // end of this polling
    DoWithActiveTasks(return_time);
    DoExpiredTimeTasks(return_time);
    DestroyClosedConnections();
  }
  // LOG(logger::kDebug, "The event loop in thread(%lu) is stopping.",
  //     ::pthread_self());
  for (auto& [_, connection] : connection_map_) {
    delete connection;
  }
  connection_map_.clear();
}

Connecting* EventManager::InsertNewConnection(int socket_fd,
                                              const NetAddress& local_address,
                                              const NetAddress& peer_address) {
  Connecting* ref_conn = nullptr;
  {
    LockGuard lock_guard(connection_map_mutex_lock_);
    connection_map_[socket_fd] =
        new Connecting(this, socket_fd, local_address, peer_address);
    ref_conn = connection_map_[socket_fd];
  }
  // LOG(logger::kDebug,
  //     "Create a new connection with fd(%d) between local net address "
  //     "(IP(%s), Port(%s)) and peer net address (IP(%s), Port(%s)).",
  //     socket_fd, local_address.GetIp().c_str(),
  //     std::to_string(local_address.GetPort()).c_str(),
  //     peer_address.GetIp().c_str(),
  //     std::to_string(peer_address.GetPort()).c_str());
  return ref_conn;
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
                                 TimePoint start_time_point,
                                 std::function<bool()> IsContinue) {
  TimePoint time_point{interval_microseconds, start_time_point, true};
  // Check if the function which decides whether to continue the cycle should be
  // set (for repeatable condition)
  if (IsContinue) {
    time_point.SetTaskContinueCallback(std::move(IsContinue));
  }
  timer_.AddTimeTask(std::move(time_point), std::move(TimeTask));
}

void EventManager::DeleteConnection(int fd) {
  LockGuard lock_guard_cf(closed_fds_lock_);
  closed_fds_.insert(fd);
}

void EventManager::DoWithActiveTasks(TimePoint return_time) {
  for (auto active_event : active_events_) {
    active_event->Work(return_time);
  }
  active_events_.clear();
}
void EventManager::DoExpiredTimeTasks(TimePoint return_time) {
  Timer::ExpiredTimeTasks expired_time_tasks = timer_.GetExpiredTimeTasks();
  for (auto& expired_time_task : expired_time_tasks) {
    auto ExpiredTimeCallback = expired_time_task.second;
    if (ExpiredTimeCallback) {
      ExpiredTimeCallback();
    }
    auto context = expired_time_task.first.GetContext();
    if (0 != context) {  // If this time task is periodic, register it again
                         // (just for the next time)
      auto IsContinue = expired_time_task.first.GetTaskContinueCallback();
      if (IsContinue) {  // If this periodic time task has a conditional
                         // judgement rule which can decide whether the time
                         // task should be done again, just judge it and take
                         // the corresponding action
        if (IsContinue()) {
          RunEveryUntil(context, std::move(ExpiredTimeCallback), return_time,
                        std::move(IsContinue));
        }
      } else {
        RunEveryUntil(context, std::move(ExpiredTimeCallback), return_time);
      }
    }
  }
}
void EventManager::DestroyClosedConnections() {
  LockGuard lock_guard(closed_fds_lock_);
  for (auto fd : closed_fds_) {
    Connecting* connection = nullptr;
    {
      LockGuard lock_guard(connection_map_mutex_lock_);
      if (connection_map_.count(fd) && connection_map_[fd]->IsDisconnected()) {
        connection = connection_map_[fd];
        connection_map_.erase(fd);
      }
    }
    delete connection;
  }
  closed_fds_.clear();
}
