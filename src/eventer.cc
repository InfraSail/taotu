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

#include <utility>

#include "logger.h"
#include "poller.h"

#ifndef POLLRDHUP
#define POLLRDHUP 0x0000
#endif

namespace taotu {

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
}

void Eventer::Work(TimePoint tp) {
  is_handling_ = true;
  // Hung up and no data to read
  if ((in_events_ & POLLHUP) && !(in_events_ & POLLIN)) {
    if (CloseCallback_) {
      LOG_DEBUG("An I/O multiplexing event is triggered now: fd(%d) is closed.",
                fd_);
      CloseCallback_();
    }
  }
  // Invalid request
  if (in_events_ & POLLNVAL) {
    LOG_WARN("An I/O multiplexing event is triggered now: fd(%d) is not open!",
             fd_);
  }
  // Invalid request and error condition
  if (in_events_ & (POLLNVAL | POLLERR)) {
    if (ErrorCallback_) {
      LOG_ERROR(
          "An I/O multiplexing event is triggered now: fd(%d) occurs an "
          "error!!!",
          fd_);
      ErrorCallback_();
    }
  }
  // Readable, urgent (read) and half-closed
  if (in_events_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (ReadCallback_) {
      LOG_DEBUG(
          "An I/O multiplexing event is triggered now: fd(%d) is readable.",
          fd_);
      ReadCallback_(std::move(tp));
    }
  }
  // Writable
  if (in_events_ & POLLOUT) {
    if (WriteCallback_) {
      LOG_DEBUG(
          "An I/O multiplexing event is triggered now: fd(%d) is writable.",
          fd_);
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

void Eventer::RemoveMyself() {
  if (out_events_ != 4294967290) {
    out_events_ = kNoEvent;
    poller_->RemoveEventer(this);
    out_events_ = 4294967290;
  }
}

void Eventer::OnReadDone(const ReadResult& res, TimePoint tp) {
  if (res.bytes > 0) {
    if (ReadCallback_) {
      ReadCallback_(tp);
    }
  } else if (res.bytes == 0) {
    if (CloseCallback_) {
      CloseCallback_();
    }
  } else {
    if (res.err != EAGAIN && res.err != EWOULDBLOCK && res.err != EINTR &&
        ErrorCallback_) {
      ErrorCallback_();
    }
  }
}

void Eventer::OnWriteDone(const WriteResult& res) {
  if (res.err != 0) {
    if (res.err != EAGAIN && res.err != EWOULDBLOCK && res.err != EINTR &&
        ErrorCallback_) {
      ErrorCallback_();
    }
    return;
  }
  if (WriteCallback_) {
    WriteCallback_();
  }
}

void Eventer::OnAcceptDone(int fd, const struct sockaddr_storage*, socklen_t) {
  if (fd < 0 && ErrorCallback_) {
    ErrorCallback_();
  } else if (ReadCallback_) {
    ReadCallback_(TimePoint{});
  }
}

void Eventer::UpdateEvents() { poller_->ModifyEventer(this); }

}  // namespace taotu
