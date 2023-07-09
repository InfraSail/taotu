/**
 * @file poller.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Poller" which is the encapsulation of I/O
 * multiplexing.
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifdef __linux__

#include "poller.h"

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <string>

#include "eventer.h"
#include "logger.h"
#include "time_point.h"

using namespace taotu;

static const int kMaxInitPollerSize = 16;

Poller::Poller()
    : poll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
      poll_events_(kMaxInitPollerSize) {
  IsPollFdEffective();
}
Poller::~Poller() {
  if (poll_fd_ >= 0) {
    ::close(poll_fd_);
  }
}

TimePoint Poller::Poll(int timeout, EventerList* active_eventers) {
  int event_amount =
      ::epoll_wait(poll_fd_, &(*(poll_events_.begin())),
                   static_cast<int>(poll_events_.size()), timeout);
  int saved_errno = errno;
  TimePoint return_time;
  if (event_amount > 0) {
    GetActiveEventer(event_amount, active_eventers);
    if (static_cast<size_t>(event_amount) == poll_events_.size()) {
      poll_events_.resize(poll_events_.size() * 2);
    }
  } else if (0 == event_amount) {
    LOG_DEBUG("In thread(%lu), there is nothing happened.", ::pthread_self());
  } else {
    if (EINTR != saved_errno) {
      errno = saved_errno;
      LOG_ERROR(
          "In thread(%lu), errors occurred when the native poll() executing!!!",
          ::pthread_self());
    }
  }
  return return_time;
}

void Poller::AddEventer(Eventer* eventer) {
  if (!IsPollFdEffective()) {
    return;
  }
  LOG_DEBUG(
      "In thread(%lu), add fd(%d) with events(%lu) into the native poll().",
      ::pthread_self(), eventer->Fd(), eventer->Events());
  struct epoll_event poll_event {};
  ::memset(static_cast<void*>(&poll_event), 0, sizeof(poll_event));
  poll_event.events = eventer->Events();
  poll_event.data.ptr = eventer;
  int event_fd = eventer->Fd();
  if (::epoll_ctl(poll_fd_, EPOLL_CTL_ADD, event_fd, &poll_event) < 0) {
    LOG_ERROR(
        "In thread(%lu), adding fd(%d) with events(%lu) into the native"
        " poll() failed!!!",
        ::pthread_self(), eventer->Fd(), eventer->Events());
  }
}
void Poller::ModifyEventer(Eventer* eventer) {
  if (!IsPollFdEffective()) {
    return;
  }
  LOG_DEBUG(
      "In thread(%lu), modify fd(%d) with events(%lu) from the native "
      "poll().",
      ::pthread_self(), eventer->Fd(), eventer->Events());
  struct epoll_event poll_event {};
  ::memset(static_cast<void*>(&poll_event), 0, sizeof(poll_event));
  poll_event.events = eventer->Events();
  poll_event.data.ptr = eventer;
  int event_fd = eventer->Fd();
  if (::epoll_ctl(poll_fd_, EPOLL_CTL_MOD, event_fd, &poll_event) < 0) {
    LOG_ERROR(
        "In thread(%lu), modifying fd(%d) with events(%lu) from the native "
        "poll() failed!!!",
        ::pthread_self(), eventer->Fd(), eventer->Events());
  }
}
void Poller::RemoveEventer(Eventer* eventer) {
  if (!IsPollFdEffective()) {
    return;
  }
  LOG_DEBUG(
      "In thread(%lu), remove fd(%d) from the native "
      "poll().",
      ::pthread_self(), eventer->Fd());
  struct epoll_event poll_event {};
  ::memset(static_cast<void*>(&poll_event), 0, sizeof(poll_event));
  poll_event.events = eventer->Events();
  poll_event.data.ptr = eventer;
  int event_fd = eventer->Fd();
  if (::epoll_ctl(poll_fd_, EPOLL_CTL_DEL, event_fd, &poll_event) < 0) {
    LOG_ERROR(
        "In thread(%lu), removing fd(%d) from the native poll() failed!!!",
        ::pthread_self(), eventer->Fd());
  }
}

void Poller::GetActiveEventer(int event_amount,
                              EventerList* active_eventers) const {
  for (size_t i = 0; i < event_amount; ++i) {
    auto eventer = static_cast<Eventer*>(poll_events_[i].data.ptr);
    eventer->ReceiveEvents(poll_events_[i].events);
    active_eventers->emplace_back(eventer);
  }
}

bool Poller::IsPollFdEffective() const {
  if (poll_fd_ < 0) {
    LOG_ERROR(
        "In thread(%lu), file descriptor the native poll() is not effective!!!",
        ::pthread_self());
    return false;
  }
  return true;
}

#else

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "eventer.h"
#include "logger.h"
#include "poller.h"
#include "time_point.h"

using namespace taotu;

static const int kMaxInitPollerSize = 16;

Poller::Poller() = default;
Poller::~Poller() = default;

TimePoint Poller::Poll(int timeout, EventerList* active_eventers) {
  int event_amount = 0;
  PollEventList poll_events;
  {
    LockGuard lock_guard(event_lock_);
    nfds_t poll_events_size = static_cast<nfds_t>(poll_events_.size());
    poll_events.resize(poll_events_.size());
    for (size_t i = 0; i < static_cast<size_t>(poll_events_size); ++i) {
      poll_events[i].fd = poll_events_[i].fd;
      poll_events[i].events = poll_events_[i].events;
      poll_events[i].revents = poll_events_[i].revents;
    }
  }
  event_amount = ::poll(&(*poll_events.begin()),
                        static_cast<nfds_t>(poll_events.size()), timeout);
  int saved_errno = errno;
  TimePoint return_time;
  if (event_amount > 0) {
    GetActiveEventer(event_amount, poll_events, active_eventers);
  } else if (0 == event_amount) {
    LOG_DEBUG("In thread(%lu), there is nothing happened.", ::pthread_self());
  } else {
    if (EINTR != saved_errno) {
      errno = saved_errno;
      LOG_ERROR(
          "In thread(%lu), errors occurred when the native poll() executing!!!",
          ::pthread_self());
    }
  }
  return return_time;
}

void Poller::AddEventer(Eventer* eventer) {
  LOG_DEBUG(
      "In thread(%lu), add fd(%d) with events(%lu) into the native poll().",
      ::pthread_self(), eventer->Fd(), eventer->Events());
  struct pollfd poll_event {};
  ::memset(static_cast<void*>(&poll_event), 0, sizeof(poll_event));
  poll_event.fd = eventer->Fd();
  poll_event.events = static_cast<short>(eventer->Events());
  poll_event.revents = 0;
  LockGuard lock_guard(event_lock_);
  poll_events_.push_back(poll_event);
  eventer->SetIndex(static_cast<int>(poll_events_.size()) - 1);
  eventers_[poll_event.fd] = eventer;
}
void Poller::ModifyEventer(Eventer* eventer) {
  LOG_DEBUG(
      "In thread(%lu), modify fd(%d) with events(%lu) from the native "
      "poll().",
      ::pthread_self(), eventer->Fd(), eventer->Events());
  LockGuard lock_guard(event_lock_);
  struct pollfd& poll_event = poll_events_[eventer->GetIndex()];
  poll_event.fd = eventer->Fd();
  poll_event.events = static_cast<short>(eventer->Events());
  poll_event.revents = 0;
  if (eventer->HasNoEvent()) {
    // Ignore it
    poll_event.fd = -eventer->Fd() - 1;
  }
}
void Poller::RemoveEventer(Eventer* eventer) {
  LOG_DEBUG(
      "In thread(%lu), remove fd(%d) from the native "
      "poll().",
      ::pthread_self(), eventer->Fd());
  int index = eventer->GetIndex();
  LockGuard lock_guard(event_lock_);
  eventers_.erase(eventer->Fd());
  if (static_cast<size_t>(index) != poll_events_.size() - 1) {
    int last_eventer_fd = poll_events_.back().fd;
    std::iter_swap(poll_events_.begin() + index, poll_events_.end() - 1);
    if (last_eventer_fd < 0) {
      last_eventer_fd = -last_eventer_fd - 1;
    }
    eventers_[last_eventer_fd]->SetIndex(index);
  }
  poll_events_.pop_back();
}

void Poller::GetActiveEventer(int event_amount,
                              const PollEventList active_events,
                              EventerList* active_eventers) const {
  LockGuard lock_guard(event_lock_);
  for (auto itr = active_events.cbegin();
       itr != active_events.cend() && event_amount > 0; ++itr) {
    if (itr->revents > 0) {
      --event_amount;
      auto evt = eventers_.find(itr->fd);
      auto& eventer = evt->second;
      eventer->ReceiveEvents(itr->revents);
      active_eventers->push_back(eventer);
    }
  }
}

#endif
