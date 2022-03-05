/**
 * @file eventer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Eventer" which is "single event operator".
 * @date 2021-11-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "eventer.h"

#include <poll.h>

#include <utility>

#include "logger.h"
#include "poller.h"

using namespace taotu;

Eventer::Eventer(Poller* poller, int fd)
    : poller_(poller),
      fd_(fd),
      in_events_(0x0000),
      out_events_(0x0000),
      is_handling_(false) {
  poller_->AddEventer(this);
}
Eventer::~Eventer() {
  while (is_handling_) {
  }
  RemoveMyself();
  poller_ = nullptr;
}

void Eventer::Work(TimePoint tp) {
  is_handling_ = true;
  // Hung up and no data to read
  if ((in_events_ & EPOLLHUP) && !(in_events_ & EPOLLIN)) {
    if (CloseCallback_) {
      LOG(logger::kDebug, "An I/O multiplexing event is triggered now: fd(" +
                              std::to_string(fd_) + ") is closed.");
      CloseCallback_();
    }
  }
  // Invalid request
  if (in_events_ & POLLNVAL) {
    LOG(logger::kWarn, "An I/O multiplexing event is triggered now: fd(" +
                           std::to_string(fd_) + ") is not open!");
  }
  // Invalid request and error condition
  if (in_events_ & (POLLNVAL | EPOLLERR)) {
    if (ErrorCallback_) {
      LOG(logger::kError, "An I/O multiplexing event is triggered now: fd(" +
                              std::to_string(fd_) + ") occurs an error!!!");
      ErrorCallback_();
    }
  }
  // Readable, urgent (read) and half-closed
  if (in_events_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
    if (ReadCallback_) {
      LOG(logger::kDebug, "An I/O multiplexing event is triggered now: fd(" +
                              std::to_string(fd_) + ") is readable.");
      ReadCallback_(tp);
    }
  }
  // Writable
  if (in_events_ & EPOLLOUT) {
    if (WriteCallback_) {
      LOG(logger::kDebug, "An I/O multiplexing event is triggered now: fd(" +
                              std::to_string(fd_) + ") is writable.");
      WriteCallback_();
    }
  }
  is_handling_ = false;
}

void Eventer::EnableReadEvents() {
  out_events_ |= kReadEvents;
  UpdateEvents();
}
void Eventer::DisableReadEvents() {
  out_events_ &= ~kReadEvents;
  UpdateEvents();
}
void Eventer::EnableWriteEvents() {
  out_events_ |= kWriteEvents;
  UpdateEvents();
}
void Eventer::DisableWriteEvents() {
  out_events_ &= ~kWriteEvents;
  UpdateEvents();
}
void Eventer::DisableAllEvents() {
  out_events_ = kNoEvent;
  UpdateEvents();
}

void Eventer::RemoveMyself() { poller_->RemoveEventer(this); }

void Eventer::UpdateEvents() { poller_->ModifyEventer(this); }
