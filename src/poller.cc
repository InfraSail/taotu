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
  TimePoint return_time;
  int saved_errno = errno;
  if (event_amount > 0) {
    GetActiveEventer(event_amount, active_eventers);
    if (static_cast<size_t>(event_amount) == poll_events_.size()) {
      poll_events_.resize(poll_events_.size() * 2);
    }
  } else if (0 == event_amount) {
    LOG(logger::kWarn, "In thread(%lu), there is nothing happened!",
        ::pthread_self());
  } else {
    if (EINTR != saved_errno) {
      errno = saved_errno;
      LOG(logger::kError,
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
  // LOG(logger::kDebug,
  //     "In thread(%lu), add fd(%d) with events(%lu) into the native poll().",
  //     ::pthread_self(), eventer->Fd(), eventer->Events());
  struct epoll_event poll_event;
  ::memset(static_cast<void*>(&poll_event), 0, sizeof(poll_event));
  poll_event.events = eventer->Events();
  poll_event.data.ptr = eventer;
  int event_fd = eventer->Fd();
  if (::epoll_ctl(poll_fd_, EPOLL_CTL_ADD, event_fd, &poll_event) < 0) {
    LOG(logger::kError,
        "In thread(%lu), adding fd(%d) with events(%lu) into the native"
        " poll() failed!!!",
        ::pthread_self(), eventer->Fd(), eventer->Events());
  }
}
void Poller::ModifyEventer(Eventer* eventer) {
  if (!IsPollFdEffective()) {
    return;
  }
  // LOG(logger::kDebug,
  //     "In thread(%lu), modify fd(%d) with events(%lu) from the native "
  //     "poll().",
  //     ::pthread_self(), eventer->Fd(), eventer->Events());
  struct epoll_event poll_event;
  ::memset(static_cast<void*>(&poll_event), 0, sizeof(poll_event));
  poll_event.events = eventer->Events();
  poll_event.data.ptr = eventer;
  int event_fd = eventer->Fd();
  if (::epoll_ctl(poll_fd_, EPOLL_CTL_MOD, event_fd, &poll_event) < 0) {
    LOG(logger::kError,
        "In thread(%lu), modifying fd(%d) with events(%lu) from the native "
        "poll() failed!!!",
        ::pthread_self(), eventer->Fd(), eventer->Events());
  }
}
void Poller::RemoveEventer(Eventer* eventer) {
  if (!IsPollFdEffective()) {
    return;
  }
  // LOG(logger::kDebug, "In thread(%lu), remove fd(%d) from the native "
  //     "poll().",
  //     ::pthread_self(), eventer->Fd());
  struct epoll_event poll_event;
  ::memset(static_cast<void*>(&poll_event), 0, sizeof(poll_event));
  poll_event.events = eventer->Events();
  poll_event.data.ptr = eventer;
  int event_fd = eventer->Fd();
  if (::epoll_ctl(poll_fd_, EPOLL_CTL_DEL, event_fd, &poll_event) < 0) {
    LOG(logger::kError,
        "In thread(%lu), removing fd(%d) from the native poll() failed!!!",
        ::pthread_self(), eventer->Fd());
  }
}

void Poller::GetActiveEventer(int event_amount,
                              EventerList* active_eventers) const {
  for (int i = 0; i < event_amount; ++i) {
    auto eventer = static_cast<Eventer*>(poll_events_[i].data.ptr);
    eventer->ReceiveEvents(poll_events_[i].events);
    active_eventers->emplace_back(eventer);
  }
}

bool Poller::IsPollFdEffective() const {
  if (poll_fd_ < 0) {
    LOG(logger::kError,
        "In thread(%lu), file descriptor the native poll() is not effective!!!",
        ::pthread_self());
    return false;
  }
  return true;
}
