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

using namespace taotu;

Eventer::Eventer(EventManager* event_manager, int fd)
    : event_manager_(event_manager),
      fd_(fd),
      in_events_(0x0000),
      out_events_(0x0000),
      is_handling_(false) {}
Eventer::~Eventer() {
  while (is_handling_) {
  }
}

void Eventer::Work(TimePoint tp) {
  is_handling_ = true;
  // Hung up and no data to read
  if ((in_events_ & 0x010) && !(in_events_ & 0x001)) {
    if (CloseCallback_) {
      LOG(logger::kDebug,
          "An I/O multiplexing event is triggered now: The fd(" +
              std::to_string(fd_) + ") is closed.");
      CloseCallback_();
    }
  }
  // Invalid request
  if (in_events_ & 0x020) {
    LOG(logger::kWarn, "An I/O multiplexing event is triggered now: The fd(" +
                           std::to_string(fd_) + ") is not open!");
  }
  // Invalid request and error condition
  if (in_events_ & (0x020 | 0x008)) {
    if (ErrorCallback_) {
      LOG(logger::kError,
          "An I/O multiplexing event is triggered now: The fd(" +
              std::to_string(fd_) + ") occurs an error!!!");
      ErrorCallback_();
    }
  }
  // Readable, urgent (read) and half-closed
  if (in_events_ & (0x001 | 0x002 | 0x2000)) {
    if (ReadCallback_) {
      LOG(logger::kDebug,
          "An I/O multiplexing event is triggered now: The fd(" +
              std::to_string(fd_) + ") is readable.");
      ReadCallback_(tp);
    }
  }
  // Writable
  if (in_events_ & 0x004) {
    if (WriteCallback_) {
      LOG(logger::kDebug,
          "An I/O multiplexing event is triggered now: The fd(" +
              std::to_string(fd_) + ") is writable.");
      WriteCallback_();
    }
  }
  is_handling_ = false;
}

void Eventer::RegisterReadCallBack(ReadCallback cb) {
  ReadCallback_ = std::move(cb);
}
void Eventer::RegisterWriteCallback(NormalCallback cb) {
  WriteCallback_ = std::move(cb);
}
void Eventer::RegisterCloseCallback(NormalCallback cb) {
  CloseCallback_ = std::move(cb);
}
void Eventer::RegisterErrorCallback(NormalCallback cb) {
  ErrorCallback_ = std::move(cb);
}

int Eventer::Fd() const { return fd_; }
int Eventer::Events() const { return out_events_; }

void Eventer::ReceiveEvents(int in_events) { in_events_ = in_events; }

bool Eventer::HasNoEvent() const { return out_events_ == kNoEvent; }
bool Eventer::HasReadEvents() const { return out_events_ & kReadEvents; }
bool Eventer::HasWriteEvents() const { return out_events_ & kWriteEvents; }

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

EventManager* Eventer::HostEventer() { return event_manager_; }

void Eventer::RemoveMyself() { event_manager_->RemoveEventer(this); }

void Eventer::UpdateEvents() { event_manager_->UpdateEventer(this); }
