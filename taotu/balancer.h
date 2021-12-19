/**
 * @file balancer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-17
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_BALANCER_H_
#define TAOTU_TAOTU_BALANCER_H_

#include <vector>

namespace taotu {

class Reactor;
class EventManager;

enum BalancerStrategy { kRoundRobin = 0, kMinEvents = 1 };

/**
 * @brief  // TODO:
 *
 */
class Balancer {
 public:
  Balancer(Reactor::EventManagers& event_managers,
           int strategy = BalancerStrategy::kMinEvents);

  void SetStrategy(int strategy) { strategy_ = strategy; }

  EventManager* PickOneEventManager();

 private:
  Reactor::EventManagers& event_managers_;
  int strategy_;
  int cursor_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_BALANCER_H_
