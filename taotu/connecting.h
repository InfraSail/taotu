/**
 * @file connecting.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-27
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_CONNECTING_H_
#define TAOTU_TAOTU_CONNECTING_H_

#include "eventer.h"
#include "net_address.h"
#include "non_copyable_movable.h"
#include "socketer.h"

namespace taotu {

class Poller;
class Socketer;
class Eventer;

/**
 * @brief  // TODO:
 *
 */
class Connecting : NonCopyableMovable {
 public:
  Connecting(Poller* poller, int socket_fd, const NetAddress& local_address,
             const NetAddress& peer_address);

 private:
  Socketer socketer_;
  Eventer eventer_;
  NetAddress local_address_;
  NetAddress peer_address_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_CONNECTING_H_
