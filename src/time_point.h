/**
 * @file time_point.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "TimePoint" which is the encapsulation of one
 * time point for the timer.
 * @date 2021-12-06
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_TIME_POINT_H_
#define TAOTU_SRC_TIME_POINT_H_

#include <stdint.h>
#include <sys/time.h>

#include <functional>

namespace taotu {

/**
 * @brief "TimePoint" encapsulates some operations of one time point in order to
 * be used in the timer more easily.
 *
 */
class TimePoint {
 public:
  // Current time point
  TimePoint();

  explicit TimePoint(int64_t duration_microseconds, bool repeated = false);

  TimePoint(int64_t duration_microseconds, const TimePoint& start_time_point,
            bool repeated = false);

  // Get the time point of next setting (for repeatable condition)
  int64_t GetContext() const { return context_; }

  // Get the time point in microsecond
  int64_t GetMicroseconds() const;

  // Get the time point in millisecond
  int64_t GetMillisecond() const;

  bool operator<(const TimePoint& tp) const {
    return time_point_microseconds_ < tp.time_point_microseconds_;
  }
  bool operator<=(const TimePoint& tp) const {
    return time_point_microseconds_ <= tp.time_point_microseconds_;
  }
  bool operator==(const TimePoint& tp) const {
    return time_point_microseconds_ == tp.time_point_microseconds_;
  }

  // Set the function which decides whether to continue the cycle (for
  // repeatable condition)
  void SetTaskContinueCallback(std::function<bool()> IsContinue);
  std::function<bool()> GetTaskContinueCallback() const;

  // Get current time point
  static int64_t FNow();

 private:
  // The time point in microsecond saved
  int64_t time_point_microseconds_;

  // The time point of next setting (for repeatable condition)
  int64_t context_;

  // The function which decides whether to continue the cycle (for repeatable
  // condition)
  std::function<bool()> IsContinue_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_TIME_POINT_H_
