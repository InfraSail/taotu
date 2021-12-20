/**
 * @file time_point.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-06
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "time_point.h"

#include <utility>

using namespace taotu;

TimePoint::TimePoint() : time_point_micro_seconds_(FNow()), context_(0) {}
TimePoint::TimePoint(int64_t duration_micro_seconds, bool repeated)
    : time_point_micro_seconds_(FNow() + duration_micro_seconds),
      context_(repeated ? duration_micro_seconds : 0) {}

int64_t TimePoint::TimePointMicroSeconds() const {
  return time_point_micro_seconds_;
}

int TimePoint::GetMillisecond() const {
  return static_cast<int>(time_point_micro_seconds_ / 1000);
}

void TimePoint::SetTaskStopingCondition(std::function<bool()> IsStoping) {
  if (0 != context_) {
    IsStoping_ = std::move(IsStoping);
  }
}
std::function<bool()> TimePoint::GetTaskStopingCondition() const {
  if (0 != context_) {
    return IsStoping_;
  }
  return []() { return false; };
}

int64_t TimePoint::FNow() {
  struct timeval tv;
  ::gettimeofday(&tv, NULL);
  return static_cast<int64_t>(tv.tv_sec * 1000 * 1000 + tv.tv_usec);
}
