/**
 * @file time_point.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "TimePoint" which is the encapsulation of one
 * time point for the timer.
 * @date 2021-12-06
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "time_point.h"

#include <utility>

using namespace taotu;

TimePoint::TimePoint() : time_point_microseconds_(FNow()), context_(0) {}
TimePoint::TimePoint(int64_t duration_microseconds, bool repeated)
    : time_point_microseconds_(FNow() + duration_microseconds),
      context_(repeated ? duration_microseconds : 0) {}

int64_t TimePoint::GetMicroseconds() const { return time_point_microseconds_; }

int TimePoint::GetMillisecond() const {
  return static_cast<int>(time_point_microseconds_ / 1000);
}

void TimePoint::SetTaskContinueCallback(std::function<bool()> IsContinue) {
  if (0 != context_) {
    IsContinue_ = std::move(IsContinue);
  }
}
std::function<bool()> TimePoint::GetTaskContinueCallback() const {
  if (0 != context_) {
    return IsContinue_;
  }
  return std::function<bool()>{};
}

int64_t TimePoint::FNow() {
  struct timeval tv;
  ::gettimeofday(&tv, NULL);
  return static_cast<int64_t>(tv.tv_sec * 1000 * 1000 + tv.tv_usec);
}
