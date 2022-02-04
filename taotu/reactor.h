/**
 * @file reactor.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_REACTOR_H_
#define TAOTU_TAOTU_REACTOR_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "acceptor.h"
#include "net_address.h"
#include "non_copyable_movable.h"

namespace taotu {

namespace {

const uint32_t kMaxEventAmount = 600000;

}

class Balancer;

/**
 * @brief  // TODO:
 *
 */
class Reactor : NonCopyableMovable {
 public:
  typedef std::vector<EventManager*> EventManagers;

  Reactor(NetAddress& listen_address, int thread_amout = 6);
  ~Reactor();

  void Loop();

 private:
  Acceptor acceptor_;
  EventManagers event_managers_;
  std::unique_ptr<Balancer> balancer_;

  bool should_stop_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_REACTOR_H_
