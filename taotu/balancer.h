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

#include <memory>
#include <vector>

namespace taotu {

class EventManager;

enum BalancerStrategy { kRoundRobin = 0, kMinEvents = 1 };

/**
 * @brief  // TODO:
 *
 */
class Balancer {
 public:
  Balancer(std::vector<EventManager*>& event_managers,
           int strategy = BalancerStrategy::kMinEvents);

  EventManager* PickOneEventManager();

 private:
  std::vector<EventManager*>& event_managers_;
  int strategy_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_BALANCER_H_
