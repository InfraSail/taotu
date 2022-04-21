/**
 * @file timer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "Timer" which is the container of time points and
 * the corresponding tasks (callback).
 * @date 2021-12-17
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_TIMER_H_
#define TAOTU_SRC_TIMER_H_

#include <functional>
#include <map>
#include <vector>

#include "non_copyable_movable.h"
#include "spin_lock.h"
#include "time_point.h"

namespace taotu {

/**
 * @brief "Timer" can be used to manage time tasks.
 *
 */
class Timer : NonCopyableMovable {
 public:
  typedef std::function<void()> TimeCallback;
  typedef std::multimap<TimePoint, TimeCallback> TimePoints;
  typedef std::vector<std::pair<TimePoint, TimeCallback>> ExpiredTimeTasks;

  Timer() = default;
  ~Timer() = default;

  // Register a time task
  void AddTimeTask(const TimePoint& time_point, TimeCallback TimeTask);

  // Get minium time duration for next epoll waiting
  int GetMinTimeDuration() const;

  // Get a set of expired time tasks
  ExpiredTimeTasks GetExpiredTimeTasks();

 private:
  // List of all time tasks (the time points and the corresponding tasks)
  TimePoints time_points_;

  // Spin lock protecting the list of all time tasks
  mutable MutexLock mutex_lock_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_TIMER_H_
