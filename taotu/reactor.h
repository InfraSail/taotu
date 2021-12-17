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

#include <memory>
#include <vector>

#include "acceptor.h"
#include "balancer.h"
#include "non_copyable_movable.h"
#include "socket_address.h"

namespace taotu {

/**
 * @brief  // TODO:
 *
 */
class Reactor : NonCopyableMovable {
 public:
  Reactor(SocketAddress& listen_address);
  ~Reactor();

 private:
  Acceptor acceptor_;
  std::vector<EventManager*> active_events_;
  std::unique_ptr<Balancer> balancer_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_REACTOR_H_
