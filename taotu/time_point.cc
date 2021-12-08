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
TimePoint::TimePoint(int64_t duration_micro_seconds)
    : time_point_micro_seconds_(FNow() + duration_micro_seconds) {}
