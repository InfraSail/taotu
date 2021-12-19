/**
 * @file timer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-17
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_TIMER_H_
#define TAOTU_TAOTU_TIMER_H_

#include <functional>
#include <map>
#include <vector>

#include "non_copyable_movable.h"
#include "spin_lock.h"
#include "time_point.h"

namespace taotu {

class EventManager;

/**
 * @brief  // TODO:
 *
 */
class Timer : NonCopyableMovable {
 public:
  typedef std::function<void()> TimeCallback;
  typedef std::multimap<TimePoint, TimeCallback> TimePoints;
  typedef std::vector<std::pair<TimePoint, TimeCallback>> ExpiredTimeTasks;

  Timer(EventManager* event_manager);
  ~Timer() {}

  void AddTimeTask(TimePoint time_point, TimeCallback time_callback);

  int GetMinTimeSet() const {
    return time_points_.begin()->first.GetMillisecond();
  }

  ExpiredTimeTasks GetExpiredTimeTasks();

 private:
  TimePoints time_points_;
  MutexLock mutex_lock_;
  EventManager* event_manager_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_TIMER_H_
