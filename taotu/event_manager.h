/**
 * @file event_manager.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_EVENT_MANAGER_H_
#define TAOTU_TAOTU_EVENT_MANAGER_H_

#include <stdint.h>

#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "non_copyable_movable.h"
#include "poller.h"
#include "time_point.h"
#include "timer.h"

namespace taotu {

class Connectioner;

class EventManager : NonCopyableMovable {
 public:
  EventManager();
  ~EventManager();

  void Loop();

  Poller *GetPoller() { return poller_.get(); }

  // For the Balancer to pick a EventManager with lowest load
  uint32_t GetEventerAmount() { return eventer_amount_; }

  void RunAt(TimePoint time_point, Timer::TimeCallback TimeTask);
  void RunAfter(int64_t delay_seconds, Timer::TimeCallback TimeTask);
  void RunEveryUntil(
      int64_t interval_seconds, Timer::TimeCallback TimeTask,
      std::function<bool()> IsContinue = std::function<bool()>{});

  // void UpdateEventer(Eventer *eventer);
  // void RemoveEventer(Eventer *eventer);

  void DoExpiredTimeTasks();

 private:
  std::unique_ptr<Poller> poller_;
  std::vector<Connectioner> eventers_;
  std::unique_ptr<std::thread> thread_;
  Timer timer_;

  // For the Balancer to pick a EventManager with lowest load
  uint32_t eventer_amount_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_EVENT_MANAGER_H_
