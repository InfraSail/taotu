/**
 * @file filer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Filer" which is "single event operator".
 * @date 2021-11-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "filer.h"

#include <utility>

#include "logger.h"

using namespace taotu;

Filer::Filer(Eventer* eventer, int fd)
    : eventer_(eventer),
      fd_(fd),
      in_events_(0x0000),
      out_events_(0x0000),
      is_handling_(false) {}
Filer::~Filer() {
  while (is_handling_) {
  }
}

void Filer::Work(TimePoint tp) {
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

void Filer::RegisterReadCallBack(ReadCallback cb) {
  ReadCallback_ = std::move(cb);
}
void Filer::RegisterWriteCallback(NormalCallback cb) {
  WriteCallback_ = std::move(cb);
}
void Filer::RegisterCloseCallback(NormalCallback cb) {
  CloseCallback_ = std::move(cb);
}
void Filer::RegisterErrorCallback(NormalCallback cb) {
  ErrorCallback_ = std::move(cb);
}

int Filer::Fd() const { return fd_; }
int Filer::Events() const { return out_events_; }

void Filer::ReceiveEvents(int in_events) { in_events_ = in_events; }

bool Filer::HasNoEvent() const { return out_events_ == kNoEvent; }
bool Filer::HasReadEvents() const { return out_events_ & kReadEvents; }
bool Filer::HasWriteEvents() const { return out_events_ & kWriteEvents; }

void Filer::EnableReadEvents() {
  out_events_ |= kReadEvents;
  UpdateEvents();
}
void Filer::DisableReadEvents() {
  out_events_ &= ~kReadEvents;
  UpdateEvents();
}
void Filer::EnableWriteEvents() {
  out_events_ |= kWriteEvents;
  UpdateEvents();
}
void Filer::DisableWriteEvents() {
  out_events_ &= ~kWriteEvents;
  UpdateEvents();
}
void Filer::DisableAllEvents() {
  out_events_ = kNoEvent;
  UpdateEvents();
}

Eventer* Filer::HostEventer() { return eventer_; }

void Filer::RemoveMyself() { eventer_->RemoveFiler(this); }

void Filer::UpdateEvents() { eventer_->UpdateFiler(this); }
