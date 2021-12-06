/**
 * @file socketer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-11-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "socketer.h"

#include <utility>

#include "logger.h"

using namespace taotu;

Socketer::Socketer(Eventer* eventer, int fd)
    : eventer_(*eventer),
      fd_(fd),
      in_events_(0x0000),
      out_events_(0x0000),
      is_handling_(false) {}
Socketer::~Socketer() {
  while (is_handling_) {
  }
}

void Socketer::Work(/*TODO:*/) {
  is_handling_ = true;
  // Hung up and no data to read
  if ((in_events_ & 0x010) && !(in_events_ & 0x001)) {
    if (close_callback_) {
      close_callback_();
    }
  }
  // Invalid request
  if (in_events_ & 0x020) {
  }
  // Invalid request and error condition
  if (in_events_ & (0x020 | 0x008)) {
    if (error_callback_) {
      error_callback_();
    }
  }
  // Readable, urgent (read) and half-closed
  if (in_events_ & (0x001 | 0x002 | 0x2000)) {
    if (read_callback_) {
      read_callback_(/*TODO:*/);
    }
  }
  // Writable
  if (in_events_ & 0x004) {
    if (write_callback_) {
      write_callback_();
    }
  }
  is_handling_ = false;
}

void Socketer::RegisterReadCallBack(ReadCallback cb) {
  read_callback_ = std::move(cb);
}
void Socketer::RegisterWriteCallback(NormalCallback cb) {
  write_callback_ = std::move(cb);
}
void Socketer::RegisterCloseCallback(NormalCallback cb) {
  close_callback_ = std::move(cb);
}
void Socketer::RegisterErrorCallback(NormalCallback cb) {
  error_callback_ = std::move(cb);
}

int Socketer::Fd() const { return fd_; }
int Socketer::Events() const { return out_events_; }

void Socketer::ReceiveEvents(int in_events) { in_events_ = in_events; }

bool Socketer::HasNoEvent() const { return out_events_ == kNoEvent; }
bool Socketer::HasReadEvents() const { return out_events_ & kReadEvents; }
bool Socketer::HasWriteEvents() const { return out_events_ & kWriteEvents; }

void Socketer::EnableReadEvents() {
  out_events_ |= kReadEvents;
  UpdateEvents();
}
void Socketer::DisableReadEvents() {
  out_events_ &= ~kReadEvents;
  UpdateEvents();
}
void Socketer::EnableWriteEvents() {
  out_events_ |= kWriteEvents;
  UpdateEvents();
}
void Socketer::DisableWriteEvents() {
  out_events_ &= ~kWriteEvents;
  UpdateEvents();
}
void Socketer::ClearAllEvents() {
  out_events_ = kNoEvent;
  UpdateEvents();
}

Eventer* Socketer::HostEventer() { return &eventer_; }

void Socketer::RemoveMyself() { eventer_.RemoveSocketer(this); }

void Socketer::UpdateEvents() { eventer_.UpdateSocketer(this); }
