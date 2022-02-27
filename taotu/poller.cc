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
#include <sys/epoll.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include "errno.h"
#include "eventer.h"
#include "logger.h"
#include "time_point.h"

using namespace taotu;

static const int kMaxInitPollerSize = 16;

Poller::Poller(uint32_t* event_amount)
    : poll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
      event_amount_(event_amount),
      poll_events_(kMaxInitPollerSize) {
  IsPollFdEffective();
}
Poller::~Poller() {
  if (poll_fd_ >= 0) {
    ::close(poll_fd_);
  }
}

TimePoint Poller::Poll(int timeout, EventerList* active_eventers) {
  // LOG(logger::kDebug,
  //     "In thread(" + std::to_string(::pthread_self()) +
  //         "), The amount of file descriptors in the native poll() is " +
  //         std::to_string(*event_amount_) + '.');
  int event_amount =
      ::epoll_wait(poll_fd_, &(*(poll_events_.begin())),
                   static_cast<int>(poll_events_.size()), timeout);
  TimePoint return_time;
  int saved_errno = errno;
  if (event_amount > 0) {
    // LOG(logger::kDebug, "In thread(" + std::to_string(::pthread_self()) +
    //                         "), there are" + std::to_string(event_amount) +
    //                         "events happened.");
    GetActiveEventer(event_amount, active_eventers);
    if (static_cast<size_t>(event_amount) == poll_events_.size()) {
      poll_events_.resize(poll_events_.size() * 2);
    }
  } else if (0 == event_amount) {
    // LOG(logger::kDebug, "In thread(" + std::to_string(::pthread_self()) +
    //                         "), there is nothing happened.");
  } else {
    if (EINTR != saved_errno) {
      errno = saved_errno;
      // LOG(logger::kError,
      //     "In thread(" + std::to_string(::pthread_self()) +
      //         "), errors occurred when the native poll() executing!!!");
    }
  }
  return return_time;
}

void Poller::AddEventer(Eventer* eventer) {
  if (!IsPollFdEffective()) {
    return;
  }
  LOG(logger::kDebug, "In thread(" + std::to_string(::pthread_self()) +
                          "), add fd(" + std::to_string(eventer->Fd()) +
                          ") with events(" + std::to_string(eventer->Events()) +
                          ") into the native poll().");
  struct epoll_event poll_event;
  ::memset(static_cast<void*>(&poll_event), 0, sizeof(poll_event));
  poll_event.events = eventer->Events();
  poll_event.data.ptr = eventer;
  int event_fd = eventer->Fd();
  if (::epoll_ctl(poll_fd_, EPOLL_CTL_ADD, event_fd, &poll_event) < 0) {
    LOG(logger::kError, "In thread(" + std::to_string(::pthread_self()) +
                            "), adding fd(" + std::to_string(eventer->Fd()) +
                            ") with events(" +
                            std::to_string(eventer->Events()) +
                            ") into the native poll() failed!!!");
  }
}
void Poller::ModifyEventer(Eventer* eventer) {
  if (!IsPollFdEffective()) {
    return;
  }
  LOG(logger::kDebug, "In thread(" + std::to_string(::pthread_self()) +
                          "), modify fd(" + std::to_string(eventer->Fd()) +
                          ") with events(" + std::to_string(eventer->Events()) +
                          ") from the native poll().");
  struct epoll_event poll_event;
  ::memset(static_cast<void*>(&poll_event), 0, sizeof(poll_event));
  poll_event.events = eventer->Events();
  poll_event.data.ptr = eventer;
  int event_fd = eventer->Fd();
  if (::epoll_ctl(poll_fd_, EPOLL_CTL_MOD, event_fd, &poll_event) < 0) {
    LOG(logger::kError, "In thread(" + std::to_string(::pthread_self()) +
                            "), modifying fd(" + std::to_string(eventer->Fd()) +
                            ") with events(" +
                            std::to_string(eventer->Events()) +
                            ") from the native poll() failed!!!");
  }
}
void Poller::RemoveEventer(Eventer* eventer) {
  if (!IsPollFdEffective()) {
    return;
  }
  LOG(logger::kDebug, "In thread(" + std::to_string(::pthread_self()) +
                          "), remove fd(" + std::to_string(eventer->Fd()) +
                          ") from the native poll().");
  struct epoll_event poll_event;
  ::memset(static_cast<void*>(&poll_event), 0, sizeof(poll_event));
  poll_event.events = eventer->Events();
  poll_event.data.ptr = eventer;
  int event_fd = eventer->Fd();
  if (::epoll_ctl(poll_fd_, EPOLL_CTL_DEL, event_fd, &poll_event) < 0) {
    LOG(logger::kError, "In thread(" + std::to_string(::pthread_self()) +
                            "), removing fd(" + std::to_string(eventer->Fd()) +
                            ") from the native poll() failed!!!");
  }
}

void Poller::GetActiveEventer(int event_amount,
                              EventerList* active_eventers) const {
  for (int i = 0; i < event_amount; ++i) {
    Eventer* eventer = static_cast<Eventer*>(poll_events_[i].data.ptr);
    eventer->ReceiveEvents(poll_events_[i].events);
    active_eventers->push_back(eventer);
  }
}

bool Poller::IsPollFdEffective() const {
  if (poll_fd_ < 0) {
    LOG(logger::kError,
        "In thread(" + std::to_string(::pthread_self()) +
            "), file descriptor the native poll() is not effective!!!");
    return false;
  }
  return true;
}
