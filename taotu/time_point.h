/**
 * @file time_point.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-06
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

#ifndef TAOTU_TAOTU_TIME_POINT_H_
#define TAOTU_TAOTU_TIME_POINT_H_

namespace taotu {

/**
 * @brief
 *
 */
class TimePoint {
 private:
  static int64_t FNow();

 public:
  TimePoint();
  TimePoint(int64_t duration_micro_seconds);

  int64_t time_point_micro_seconds_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_TIME_POINT_H_
