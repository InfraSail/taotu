/**
 * @file balancer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "Balancer" which can balance the number of
 * "Eventer"s in each I/O thread("EventManager") as much as possible.
 * @date 2021-12-17
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_BALANCER_H_
#define TAOTU_TAOTU_BALANCER_H_

#include <vector>

#include "reactor_manager.h"

namespace taotu {

enum BalancerStrategy {
  kRoundRobin = 0,  // Use "Round Robin" strategy
  kMinEvents = 1  // Use the strategy which always picks the I/O thread holding
                  // least "Eventer"s
};

/**
 * @brief "Balancer" balances the load of file handles distributed to each I/O
 * thread("EventManager") according to the specified strategy.
 *
 */
class Balancer {
 public:
  Balancer(ReactorManager::EventManagers* event_managers,
           int strategy = BalancerStrategy::kMinEvents);
  ~Balancer() { event_managers_ = nullptr; }

  // Set the strategy of balancing the load of file handles distributed to each
  // I/O thread("EventManager")
  void SetStrategy(int strategy) { strategy_ = strategy; }

  // Get one "EventManager" in the least busy I/O thread
  EventManager* PickOneEventManager();

 private:
  // Weak reference from the set of "EventManager"s from "Reactor" in the main
  // thread
  ReactorManager::EventManagers* event_managers_;

  // Strategy of balancing the load of file handles distributed to each I/O
  // thread("EventManager")
  int strategy_;

  // The mark of the index of the chosen "EventManager" in "event_managers_"
  int cursor_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_BALANCER_H_
