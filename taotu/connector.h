/**
 * @file connector.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-12
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_CONNECTOR_H_
#define TAOTU_TAOTU_CONNECTOR_H_

#include <functional>

#include "connecting.h"
#include "eventer.h"
#include "net_address.h"
#include "non_copyable_movable.h"
#include "poller.h"

namespace taotu {

/**
 * @brief  // TODO:
 *
 */
class Connector : NonCopyableMovable {
 public:
  typedef std::function<void(int)> NewConnectionCallback;

  Connector(Poller* poller, const NetAddress& server_address);
  ~Connector() {}

  void Connect();
  void DoConnecting(int socket_fd);
  void DoRetrying(int socket_fd);

  void DoWriting();
  void DoWithError();

 private:
  typedef std::unique_ptr<Eventer> EventerPtr;
  enum State { kDisconnected, kConnecting, kConnected };

  void SetState(State state) { state_ = state; }

  Poller* poller_;
  NetAddress server_address_;
  State state_;
  NewConnectionCallback NewConnectionCallback_;

  EventerPtr eventer_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_CONNECTOR_H_
