/**
 * @file filer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-11-28
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "filer.h"

#include <unistd.h>

#include <utility>

#include "logger.h"

using namespace taotu;

Filer::Filer(Eventer* eventer, int fd)
    : eventer_(*eventer),
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
    if (close_callback_) {
      LOG(logger::kDebug,
          "An I/O multiplexing event is triggered now: The fd(" +
              std::to_string(fd_) + ") is closed.");
      close_callback_();
    }
  }
  // Invalid request
  if (in_events_ & 0x020) {
    LOG(logger::kWarn, "An I/O multiplexing event is triggered now: The fd(" +
                           std::to_string(fd_) + ") is not open!");
  }
  // Invalid request and error condition
  if (in_events_ & (0x020 | 0x008)) {
    if (error_callback_) {
      LOG(logger::kError,
          "An I/O multiplexing event is triggered now: The fd(" +
              std::to_string(fd_) + ") occurs an error!!!");
      error_callback_();
    }
  }
  // Readable, urgent (read) and half-closed
  if (in_events_ & (0x001 | 0x002 | 0x2000)) {
    if (read_callback_) {
      LOG(logger::kDebug,
          "An I/O multiplexing event is triggered now: The fd(" +
              std::to_string(fd_) + ") is readable.");
      read_callback_(tp);
    }
  }
  // Writable
  if (in_events_ & 0x004) {
    if (write_callback_) {
      LOG(logger::kDebug,
          "An I/O multiplexing event is triggered now: The fd(" +
              std::to_string(fd_) + ") is writable.");
      write_callback_();
    }
  }
  is_handling_ = false;
}

void Filer::RegisterReadCallBack(ReadCallback cb) {
  read_callback_ = std::move(cb);
}
void Filer::RegisterWriteCallback(NormalCallback cb) {
  write_callback_ = std::move(cb);
}
void Filer::RegisterCloseCallback(NormalCallback cb) {
  close_callback_ = std::move(cb);
}
void Filer::RegisterErrorCallback(NormalCallback cb) {
  error_callback_ = std::move(cb);
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

Eventer* Filer::HostEventer() { return &eventer_; }

void Filer::RemoveMyself() { eventer_.RemoveFiler(this); }

void Filer::UpdateEvents() { eventer_.UpdateFiler(this); }
