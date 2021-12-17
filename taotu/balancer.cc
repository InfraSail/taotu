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

using namespace taotu;

Balancer::Balancer(std::vector<EventManager*>& event_managers,
                   int strategy = BalancerStrategy::kMinEvents)
    : event_managers_(event_managers), strategy_(strategy) {}

EventManager* Balancer::PickOneEventManager() {
  int evt_mng_num = event_managers_.size();
  switch (strategy_) {
    case BalancerStrategy::kRoundRobin:
    case BalancerStrategy::kMinEvents:
  }
}
