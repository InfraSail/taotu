/**
 * @file balancer.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-17
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "balancer.h"

#include <algorithm>

#include "event_manager.h"
#include "logger.h"
#include "reactor.h"

using namespace taotu;

Balancer::Balancer(Reactor::EventManagers& event_managers,
                   int strategy = BalancerStrategy::kMinEvents)
    : event_managers_(event_managers), strategy_(strategy), cursor_(0) {}

EventManager* Balancer::PickOneEventManager() {
  int evt_mng_num = event_managers_.size();
  switch (strategy_) {
    case BalancerStrategy::kRoundRobin:
      if (++cursor_ >= evt_mng_num) {
        cursor_ = 0;
      }
      break;
    case BalancerStrategy::kMinEvents:
      int pos = 0;
      int min_evts = event_managers_[pos]->GetEventerAmount();
      for (int i = 1; i < evt_mng_num; ++i) {
        if (event_managers_[i]->GetEventerAmount() < min_evts) {
          pos = i;
          min_evts = static_cast<int>(event_managers_[pos]->GetEventerAmount());
        }
        cursor_ = pos;
      }
    default:
      LOG(logger::kError, "Wrong strategy of Reactor's Balancer!!!");
  }
  return event_managers_[cursor_];
}
