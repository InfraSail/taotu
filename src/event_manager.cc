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
#ifdef __linux__
#include <sys/eventfd.h>
#endif
#include <unistd.h>

#include <string>
#include <utility>

#include "connecting.h"
#include "eventer.h"
#include "logger.h"
#include "spin_lock.h"
#include "timer.h"

using namespace taotu;

EventManager::EventManager(
    std::function<Connecting*(EventManager*, int, const NetAddress&,
                              const NetAddress&)>
        CreateConnectionCallback,
    std::function<void(Connecting*)> DestroyConnectionCallback)
    : poller_()
#ifdef __linux__
      ,
      wake_up_eventer_(
          &poller_,
          []() -> int {
            int event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            if (event_fd < 0) {
              LOG_ERROR("Creating wake_up_eventer fails in I/O thread(%lu)!!!",
                        ::pthread_self());
              ::exit(-1);
            }
            return event_fd;
          }())
#else
      ,
      wake_up_eventer_(nullptr)
#endif
{
#ifdef __linux__
  wake_up_eventer_.RegisterReadCallback([this](const TimePoint&) {
    uint64_t msg = 1;
    ssize_t n = ::read(this->wake_up_eventer_.Fd(),
                       reinterpret_cast<void*>(&msg), sizeof(msg));
    if (n != sizeof(msg)) {
      LOG_ERROR(
          "The wake_up_eventer in I/O thread(%lu) reads %llubytes instead of 8 "
          "bytes!!!",
          ::pthread_self(), msg);
    }
  });
  wake_up_eventer_.EnableReadEvents();
#else
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, wake_up_pipe_) < 0) {
    LOG_ERROR("Fail in socketpair!!!");
  }
  wake_up_eventer_ = new Eventer(&poller_, wake_up_pipe_[0]);
  wake_up_eventer_->RegisterReadCallback([this](const TimePoint&) {
    uint64_t msg = 1;
    ssize_t n = ::read(this->wake_up_pipe_[0], reinterpret_cast<void*>(&msg),
                       sizeof(msg));
    if (n != sizeof(msg)) {
      LOG_ERROR(
          "The wake_up_eventer in I/O thread(%lu) reads %llubytes instead of 8 "
          "bytes!!!",
          ::pthread_self(), msg);
    }
  });
  wake_up_eventer_->EnableReadEvents();
#endif
  if (CreateConnectionCallback && DestroyConnectionCallback) {
    CreateConnectionCallback_ = CreateConnectionCallback;
    DestroyConnectionCallback_ = DestroyConnectionCallback;
  }
}
EventManager::~EventManager() {
#ifdef __linux__
  wake_up_eventer_.DisableAllEvents();
  wake_up_eventer_.RemoveMyself();
  ::close(wake_up_eventer_.Fd());
#else
  wake_up_eventer_->DisableAllEvents();
  wake_up_eventer_->RemoveMyself();
  ::close(wake_up_pipe_[0]);
  ::close(wake_up_pipe_[1]);
  delete wake_up_eventer_;
#endif
  if (thread_.joinable()) {
    thread_.join();
  }
}

void EventManager::Loop() {
  thread_ = std::thread([this] { this->Work(); });
}
void EventManager::Work() {
  should_quit_ = false;
  LOG_DEBUG("The event loop in thread(%lu) is starting.", ::pthread_self());
  while (!should_quit_) {
    auto return_time =
        poller_.Poll(timer_.GetMinTimeDuration(),
                     &active_events_);  // Return time is the time point of the
                                        // end of this polling
    DoWithActiveTasks(return_time);
    DoExpiredTimeTasks(return_time);
    DestroyClosedConnections();
  }
  LOG_DEBUG("The event loop in thread(%lu) is stopping.", ::pthread_self());
  LockGuard lock_guard(connection_map_mutex_lock_);
  for (auto& [_, connection] : connection_map_) {
    delete connection;
  }
  connection_map_.clear();
}

Connecting* EventManager::InsertNewConnection(int socket_fd,
                                              const NetAddress& local_address,
                                              const NetAddress& peer_address) {
  Connecting* ref_conn;
  {
    LockGuard lock_guard(connection_map_mutex_lock_);
    if (CreateConnectionCallback_) {
      connection_map_[socket_fd] = CreateConnectionCallback_(
          this, socket_fd, local_address, peer_address);
    } else {
      connection_map_[socket_fd] =
          new Connecting(this, socket_fd, local_address, peer_address);
    }
    ref_conn = connection_map_[socket_fd];
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
#ifdef __linux__
  uint64_t msg = 1;
  ssize_t n = ::write(wake_up_eventer_.Fd(), reinterpret_cast<void*>(&msg),
                      sizeof(msg));
  if (n != sizeof(msg)) {
    LOG_ERROR(
        "The wake_up_eventer in I/O thread(%lu) writes %llubytes instead of 8 "
        "bytes!!!",
        ::pthread_self(), msg);
  }
#else
  uint64_t msg = 1;
  ssize_t n =
      ::write(wake_up_pipe_[1], reinterpret_cast<void*>(&msg), sizeof(msg));
  if (n != sizeof(msg)) {
    LOG_ERROR(
        "The wake_up_eventer in I/O thread(%lu) writes %llubytes instead of 8 "
        "bytes!!!",
        ::pthread_self(), msg);
  }
#endif
}

void EventManager::Quit() { should_quit_ = true; }

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
  for (auto fd : closed_fds_) {
    Connecting* connection = nullptr;
    {
      LockGuard lock_guard_cm(connection_map_mutex_lock_);
      if (connection_map_.count(fd) && connection_map_[fd]->IsDisconnected()) {
        connection = connection_map_[fd];
        connection_map_.erase(fd);
      }
    }
    if (DestroyConnectionCallback_) {
      DestroyConnectionCallback_(connection);
    } else {
      delete connection;
    }
  }
  closed_fds_.clear();
}
