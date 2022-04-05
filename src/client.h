/**
 * @file client.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "Client" which is the encapsulation of the
 * client.
 * @date 2022-01-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_SRC_CLIENT_H_
#define TAOTU_SRC_CLIENT_H_

#include <memory>
#include <utility>

#include "net_address.h"
#include "non_copyable_movable.h"
#include "reactor_manager.h"

namespace taotu {

/**
 * @brief "Client" offer APIs which can handle client-end new TCP connection
 * creation to users.
 *
 */
class Client : NonCopyableMovable {
 public:
  Client(EventManager* event_manager, const NetAddress& server_address,
         bool should_retry_);

  void SetConnectionCallback(const std::function<void(Connecting&)>& cb);
  void SetMessageCallback(
      const std::function<void(Connecting&, IoBuffer*, TimePoint)>& cb);
  void SetWriteCompleteCallback(const std::function<void(Connecting&)>& cb);

  // Try to connect to the specific net address
  void Connect();

  // Disconnect and shut down writing
  void Disconnect();

  // Stop the TCP connection (if because of acceptable exceptions in
  // hardware-level, just retry)
  void Stop();

 private:
  typedef std::unique_ptr<ClientReactorManager> ClientReactorManagerPtr;

  // Reactor manager (the "engine")
  ClientReactorManagerPtr reactor_manager_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_CLIENT_H_
