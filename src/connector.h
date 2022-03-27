/**
 * @file connector.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "Connector" which is the connector of new
 * connections requests the server and create new connections.
 * @date 2021-12-12
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_CONNECTOR_H_
#define TAOTU_SRC_CONNECTOR_H_

#include <functional>

#include "connecting.h"
#include "event_manager.h"
#include "eventer.h"
#include "net_address.h"
#include "non_copyable_movable.h"

namespace taotu {

/**
 * @brief "Connector" is dedicated to handle new connection creations and build
 * the connections in the main thread (then register it into corresponding I/O
 * threads).
 *
 */
class Connector : NonCopyableMovable {
 public:
  typedef std::function<void(int)> NewConnectionCallback;

  Connector(EventManager* event_manager, const NetAddress& server_address);
  ~Connector() {}

  // Start the connector to connect the server
  void Start();

  // Restart the connector to connect the server
  void Restart();

  // Stop the connector to connect the server (maybe retry in some conditions)
  void Stop();

  void Connect();

  // Execute when preparing for connecting
  void DoConnecting(int socket_fd);

  // Execute when retrying
  void DoRetrying(int socket_fd);

  void RegisterNewConnectionCallback(const NewConnectionCallback& cb) {
    NewConnectionCallback_ = cb;
  }

  const NetAddress& GetServerAddress() { return server_address_; }

  // Execute when create a new conncetion
  void DoWriting();

  // Execute when error happens
  void DoWithError();

 private:
  typedef std::unique_ptr<Eventer> EventerPtr;
  enum State { kDisconnected, kConnecting, kConnected };

  // After successful connecting, call it to reset because the connector's file
  // descriptot is disposable
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

#endif  // !TAOTU_SRC_CONNECTOR_H_
