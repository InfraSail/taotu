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
#include "event_manager.h"
#include "eventer.h"
#include "net_address.h"
#include "non_copyable_movable.h"

namespace taotu {

/**
 * @brief  // TODO:
 *
 */
class Connector : NonCopyableMovable {
 public:
  typedef std::function<void(int)> NewConnectionCallback;

  Connector(EventManager* event_manager, const NetAddress& server_address);
  ~Connector() {}

  void Start();
  void Restart();
  void Stop();

  void Connect();
  void DoConnecting(int socket_fd);
  void DoRetrying(int socket_fd);

  void RegisterNewConnectionCallback(const NewConnectionCallback& cb) {
    NewConnectionCallback_ = cb;
  }

  const NetAddress& GetServerAddress() { return server_address_; }

  void DoWriting();
  void DoWithError();

 private:
  typedef std::unique_ptr<Eventer> EventerPtr;
  enum State { kDisconnected, kConnecting, kConnected };

  int RemoveAndReset();

  void SetState(State state) { state_ = state; }

  EventManager* event_manager_;
  NetAddress server_address_;
  State state_;
  bool can_connect_;
  int retry_dalay_microseconds_;
  NewConnectionCallback NewConnectionCallback_;

  EventerPtr eventer_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_CONNECTOR_H_
