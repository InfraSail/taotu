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

#include "connecting.h"
#include "logger.h"
#include "spin_lock.h"
#include "timer.h"

using namespace taotu;

EventManager::EventManager() : eventer_amount_(0), poller_(&eventer_amount_) {}
EventManager::~EventManager() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

void EventManager::Loop() {
  thread_ = std::thread(std::bind(&EventManager::Work, this));
}
void EventManager::Work() {
  should_quit_ = false;
  LOG(logger::kDebug, "The event loop in a new thread is starting.");
  while (!should_quit_) {
    DoWithActiveTasks(
        poller_.Poll(timer_.GetMinTimeDurationSet(), &active_events_));
    DoExpiredTimeTasks();
    DestroyClosedConnections();
  }
  LOG(logger::kDebug, "The event loop in a new thread is stopping.");
  for (auto& it : connection_map_) {
    it.second->OnDestroying();
  }
  connection_map_.clear();
}

Connecting* EventManager::InsertNewConnection(int socket_fd,
                                              const NetAddress& local_address,
                                              const NetAddress& peer_address) {
  Connecting* ref_conn = nullptr;
  {
    LockGuard lock_guard(connection_map_mutex_lock_);
    connection_map_[socket_fd] = std::make_unique<Connecting>(
        this, socket_fd, local_address, peer_address);
    ref_conn = connection_map_[socket_fd].release();
  }
  ++eventer_amount_;
  LOG(logger::kDebug,
      "Create a new connection that fd(" + std::to_string(socket_fd) +
          ") with local net address (IP(" + local_address.GetIp() + "), Port(" +
          std::to_string(local_address.GetPort()) +
          ")) and peer net address (IP(" + peer_address.GetIp() + "), Port(" +
          std::to_string(peer_address.GetPort()) + ")).");
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
                                 std::function<bool()> IsContinue) {
  TimePoint time_point{interval_microseconds, true};
  // Check if the function which decides whether to continue the cycle should be
  // set (for repeatable condition)
  if (IsContinue) {
    time_point.SetTaskContinueCallback(std::move(IsContinue));
  }
  timer_.AddTimeTask(std::move(time_point), std::move(TimeTask));
}

void EventManager::DeleteConnection(int fd) {
  LockGuard lock_guard(connection_map_mutex_lock_);
  if (connection_map_[fd]->IsDisconnected()) {
    closed_fds_.push_back(fd);
  }
}

void EventManager::DoWithActiveTasks(TimePoint return_time) {
  for (auto active_event : active_events_) {
    active_event->Work(return_time);
    DeleteConnection(active_event->Fd());
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
  for (auto fd : closed_fds_) {
    LockGuard lock_guard(connection_map_mutex_lock_);
    connection_map_.erase(fd);
    --eventer_amount_;
  }
  closed_fds_.clear();
}
