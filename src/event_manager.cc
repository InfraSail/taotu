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

#include <pthread.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <string>
#include <utility>

#include "connecting.h"
#include "eventer.h"
#include "logger.h"
#include "spin_lock.h"
#include "timer.h"

namespace taotu {

EventManager::EventManager()
    : poller_(), thread_(), wake_up_eventer_(&poller_, []() -> int {
        int event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (event_fd < 0) {
          LOG_ERROR("Creating wake_up_eventer fails in I/O thread(%lu)!!!",
                    ::pthread_self());
          ::exit(-1);
        }
        return event_fd;
      }()) {
  wake_up_eventer_.RegisterReadCallback([this](const TimePoint&) {
    // Drain eventfd to keep event loop responsive.
    while (true) {
      uint64_t msg = 0;
      ssize_t n = ::read(this->wake_up_eventer_.Fd(),
                         reinterpret_cast<void*>(&msg), sizeof(msg));
      if (n == sizeof(msg)) {
        continue;
      }
      if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        break;
      }
      if (n != sizeof(msg)) {
        LOG_ERROR(
            "The wake_up_eventer in I/O thread(%lu) reads %lldbytes instead of "
            "8 bytes!!!",
            ::pthread_self(), static_cast<long long>(n < 0 ? -1 : n));
      }
      break;
    }
  });
  wake_up_eventer_.EnableReadEvents();
}
EventManager::~EventManager() {
  Quit();
  if (thread_ && thread_->joinable()) {
    thread_->join();
  }
  wake_up_eventer_.DisableAllEvents();
  wake_up_eventer_.RemoveMyself();
  ::close(wake_up_eventer_.Fd());
}

void EventManager::Loop() {
  std::call_once(start_once_flag_, [this]() {
    thread_ = std::make_unique<std::thread>([this] { this->Start(); });
  });
}
void EventManager::Work() {
  std::call_once(start_once_flag_, [this]() { this->Start(); });
}

void EventManager::Join() {
  if (thread_ && thread_->joinable()) {
    thread_->join();
  }
}

Connecting* EventManager::InsertNewConnection(int socket_fd,
                                              const NetAddress& local_address,
                                              const NetAddress& peer_address) {
  Connecting* ref_conn = nullptr;
  {
    LockGuard lock_guard(connection_map_mutex_lock_);
    if (CreateConnectionCallback_) {
      // CreateConnectionCallback_ returns raw pointer, wrap it in unique_ptr
      connection_map_[socket_fd].reset(CreateConnectionCallback_(
          this, socket_fd, local_address, peer_address));
    } else {
      connection_map_[socket_fd] = std::make_unique<Connecting>(
          this, socket_fd, local_address, peer_address);
    }
    ref_conn = connection_map_[socket_fd].get();
  }
  LOG_DEBUG(
      "Create a new connection with fd(%d) between local net address "
      "[ IP(%s), Port(%s) ] and peer net address [ IP(%s), Port(%s) ].",
      socket_fd, local_address.GetIp().c_str(),
      std::to_string(local_address.GetPort()).c_str(),
      peer_address.GetIp().c_str(),
      std::to_string(peer_address.GetPort()).c_str());
  return ref_conn;
}

void EventManager::RunAt(const TimePoint& time_point,
                         Timer::TimeCallback TimeTask) {
  const TimePoint& tmp_time_point = time_point;
  timer_.AddTimeTask(time_point, std::move(TimeTask));
  if (timer_.GetMinTimeDuration() >=
      static_cast<int>(tmp_time_point.GetMillisecond() -
                       TimePoint().GetMillisecond())) {
    WakeUp();
  }
}
void EventManager::RunAfter(int64_t delay_microseconds,
                            Timer::TimeCallback TimeTask) {
  TimePoint tmp_time_point{delay_microseconds};
  timer_.AddTimeTask(tmp_time_point, std::move(TimeTask));
  if (timer_.GetMinTimeDuration() >=
      static_cast<int>(tmp_time_point.GetMillisecond() -
                       TimePoint().GetMillisecond())) {
    WakeUp();
  }
}
void EventManager::RunEveryUntil(int64_t interval_microseconds,
                                 Timer::TimeCallback TimeTask,
                                 const TimePoint& start_time_point,
                                 std::function<bool()> IsContinue) {
  TimePoint time_point{interval_microseconds, start_time_point, true};
  TimePoint tmp_time_point = time_point;
  // Check if the function which decides whether to continue the cycle should be
  // set (for repeatable condition)
  if (IsContinue) {
    time_point.SetTaskContinueCallback(std::move(IsContinue));
  }
  timer_.AddTimeTask(time_point, std::move(TimeTask));
  if (timer_.GetMinTimeDuration() >=
      static_cast<int>(tmp_time_point.GetMillisecond() -
                       TimePoint().GetMillisecond())) {
    WakeUp();
  }
}

void EventManager::RunSoon(Timer::TimeCallback TimeTask) {
  timer_.AddTimeTask(TimePoint{}, std::move(TimeTask));
  WakeUp();
}

void EventManager::DeleteConnection(int fd) {
  LockGuard lock_guard(closed_fds_lock_);
  closed_fds_.insert(fd);
}

void EventManager::WakeUp() {
  uint64_t msg = 1;
  ssize_t n = ::write(wake_up_eventer_.Fd(), reinterpret_cast<void*>(&msg),
                      sizeof(msg));
  if (n != sizeof(msg)) {
    LOG_ERROR(
        "The wake_up_eventer in I/O thread(%lu) writes %llubytes instead of 8 "
        "bytes!!!",
        ::pthread_self(), msg);
  }
}

void EventManager::Quit() {
  should_quit_.store(true);
  WakeUp();
}

void EventManager::Start() {
  should_quit_.store(false);
  LOG_DEBUG("The event loop in thread(%lu) is starting.", ::pthread_self());
  while (!should_quit_.load()) {
    auto return_time =
        poller_.Poll(timer_.GetMinTimeDuration(),
                     &active_events_);  // Return time is the time point of
                                        // the end of this polling
    DoWithActiveTasks(return_time);
    DoExpiredTimeTasks(return_time);
    DestroyClosedConnections();
  }
  LOG_DEBUG("The event loop in thread(%lu) is stopping.", ::pthread_self());
  std::vector<Connecting*> connections_to_close;
  {
    LockGuard lock_guard(connection_map_mutex_lock_);
    connections_to_close.reserve(connection_map_.size());
    for (const auto& it : connection_map_) {
      if (it.second) {
        connections_to_close.push_back(it.second.get());
      }
    }
  }
  for (auto* connection : connections_to_close) {
    connection->ForceClose();
  }
  // Drain pending CQEs and destroy connections safely before leaving.
  while (true) {
    bool has_connections = false;
    bool has_closed = false;
    {
      LockGuard lock_guard_cm(connection_map_mutex_lock_);
      has_connections = !connection_map_.empty();
    }
    {
      LockGuard lock_guard_cf(closed_fds_lock_);
      has_closed = !closed_fds_.empty();
    }
    if (!has_connections && !has_closed) {
      break;
    }
    poller_.Poll(1, &active_events_);
    DoWithActiveTasks(TimePoint{});
    DestroyClosedConnections();
  }
  {
    LockGuard lock_guard(connection_map_mutex_lock_);
    connection_map_.clear();
  }
}

void EventManager::DoWithActiveTasks(const TimePoint& return_time) {
  for (auto active_event : active_events_) {
    active_event->Work(return_time);
  }
  active_events_.clear();
}
void EventManager::DoExpiredTimeTasks(const TimePoint& return_time) {
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
  LockGuard lock_guard_cf(closed_fds_lock_);
  Fds remaining_fds;
  for (auto fd : closed_fds_) {
    std::unique_ptr<Connecting> connection_ptr;
    {
      LockGuard lock_guard_cm(connection_map_mutex_lock_);
      auto it = connection_map_.find(fd);
      if (it != connection_map_.end() && it->second &&
          it->second->IsDisconnected()) {
        if (it->second->HasPendingIo()) {
          remaining_fds.insert(fd);
          continue;
        }
        connection_ptr = std::move(it->second);
        connection_map_.erase(it);
      }
    }
    if (connection_ptr) {
      if (DestroyConnectionCallback_) {
        // Release ownership to callback (callback takes ownership)
        DestroyConnectionCallback_(connection_ptr.release());
      }
      // else unique_ptr automatically deletes when going out of scope
    }
  }
  closed_fds_.swap(remaining_fds);
}

}  // namespace taotu
