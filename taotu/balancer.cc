/**
 * @file balancer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Balancer" which can balance the number of
 * "Eventer"s in each I/O thread("EventManager") as much as possible.
 * @date 2021-12-17
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "balancer.h"

#include "event_manager.h"
#include "logger.h"
#include "reactor_manager.h"

using namespace taotu;

Balancer::Balancer(ServerReactorManager::EventManagers* event_managers,
                   int strategy)
    : event_managers_(event_managers), strategy_(strategy), cursor_(0) {}

EventManager* Balancer::PickOneEventManager() {
  int evt_mng_num = event_managers_->size();
  switch (strategy_) {
    // "Round Robin"
    case BalancerStrategy::kRoundRobin:
      if (++cursor_ >= evt_mng_num) {
        cursor_ = 0;
      }
      break;
    // Pick the I/O thread holding least "Eventer"s
    case BalancerStrategy::kMinEvents:
      int pos = 0;
      int min_evts = (*event_managers_)[pos]->GetEventerAmount();
      for (int i = 1; i < evt_mng_num; ++i) {
        if ((*event_managers_)[i]->GetEventerAmount() < min_evts) {
          pos = i;
          min_evts =
              static_cast<int>((*event_managers_)[pos]->GetEventerAmount());
        }
        cursor_ = pos;
      }
      break;
  }
  return (*event_managers_)[cursor_].release();
}
