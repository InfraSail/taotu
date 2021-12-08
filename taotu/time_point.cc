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

using namespace taotu;

int64_t TimePoint::FNow() {
  struct timeval tv;
  ::gettimeofday(&tv, NULL);
  return static_cast<int64_t>(tv.tv_sec * 1000 * 1000 + tv.tv_usec);
}

TimePoint::TimePoint() : time_point_micro_seconds_(FNow()) {}
TimePoint::TimePoint(int64_t duration_micro_seconds, bool repeated)
    : time_point_micro_seconds_(FNow() + duration_micro_seconds),
      context_(repeated ? duration_micro_seconds : 0) {}

int64_t TimePoint::TimePointMicroSeconds() const {
  return time_point_micro_seconds_;
}
int64_t TimePoint::Context() const { return context_; }

bool TimePoint::operator<(const TimePoint& tp) const {
  return time_point_micro_seconds_ < tp.time_point_micro_seconds_;
}
bool TimePoint::operator==(const TimePoint& tp) const {
  return time_point_micro_seconds_ == tp.time_point_micro_seconds_;
}
