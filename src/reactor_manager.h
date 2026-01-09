/**
 * @file reactor_manager.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declarations of class "ServerReactorManager" which manages reactors in
 * the server and class "ClientReactorManager" which manages the "Reactor" in
 * the client.
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_REACTOR_MANAGER_H_
#define TAOTU_SRC_REACTOR_MANAGER_H_

#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "acceptor.h"
#include "connecting.h"
#include "connector.h"
#include "net_address.h"
#include "non_copyable_movable.h"
#include "object_pool.h"
#include "spin_lock.h"

namespace taotu {

class Balancer;

/**
 * @brief "ServerReactorManager" is the engine of the server which manages
 * almost everything including new TCP connections' creation and concurrent I/O.
 * It allows users to define what to do by some flags and different callback
 * functions.
 *
 */
class ServerReactorManager : NonCopyableMovable {
 public:
  typedef Connecting::NormalCallback NormalCallback;
  typedef Connecting::OnMessageCallback MessageCallback;

  typedef std::vector<EventManager*> EventManagers;

  ServerReactorManager(EventManagers* event_managers,
                       const NetAddress& listen_address,
                       bool should_reuse_port = false);
  ~ServerReactorManager();

  void SetConnectionCallback(const NormalCallback& cb) {
    ConnectionCallback_ = cb;
  }
  void SetMessageCallback(const MessageCallback& cb) { MessageCallback_ = cb; }
  void SetWriteCompleteCallback(const NormalCallback& cb) {
    WriteCompleteCallback_ = cb;
  }
  void SetCloseCallback(const NormalCallback& cb) { CloseCallback_ = cb; }

  // Drive the engine (push everything starting -- start all event loops)
  void Loop();

  // Create a new connection from the object pool
  template <class... Args>
  Connecting* NewOneConnectingFromObjectPool(Args... args) {
    LockGuard lock_guard(object_pool_lock_);
    return object_pool_.New(args...);
  }

  // Delete a connection from the object pool
  void DeleteOneConnectingFromObjectPool(Connecting* connecting_ptr) {
    LockGuard lock_guard(object_pool_lock_);
    object_pool_.Delete(connecting_ptr);
  }

 private:
  typedef std::unique_ptr<Acceptor> AcceptorPtr;
  typedef std::unique_ptr<Balancer> BalancerPtr;

  // Build a new TCP connection and insert it into the corresponding I/O thread
  void AcceptNewConnectionCallback(int socket_fd,
                                   const NetAddress& peer_address);

  // Event managers which are the "Reactor"s that manages events in their own
  // I/O threads
  EventManagers* event_managers_;

  // Acceptor for accepting new connections in the main thread
  AcceptorPtr acceptor_;

  // Load balancer for dispatching new connections into I/O threads
  BalancerPtr balancer_;

  // Callback function which will be called after this TCP connection creating
  // and before this TCP connection destroying
  NormalCallback ConnectionCallback_;

  // Callback function which will be called after each reading
  MessageCallback MessageCallback_;

  // Callback function which will be called after each real writing
  NormalCallback WriteCompleteCallback_;

  // Callback function which will be called when this TCP connection should be
  // closed
  NormalCallback CloseCallback_;

  // Object pool for connections
  ObjectPool<Connecting> object_pool_;

  // Spin lock protecting the object pool
  mutable MutexLock object_pool_lock_;
};

/**
 * @brief "ClientReactorManager" is the disposable engine of the client which
 * manages the almost everything of single TCP connection including new TCP
 * connection's creation and I/O. It needs a "EventManager" defined by users
 * (for sharing the "EventManager") to put in and allows users to define what to
 * do by some flags and different callback functions.
 *
 */
class ClientReactorManager
    : public NonCopyableMovable,
      public std::enable_shared_from_this<ClientReactorManager> {
 public:
  typedef Connecting::NormalCallback NormalCallback;
  typedef Connecting::OnMessageCallback MessageCallback;

  ClientReactorManager(EventManager* event_manager,
                       const NetAddress& server_address);
  ~ClientReactorManager();

  // Try to connect to the specific net address
  void Connect();

  // Disconnect and shut down writing
  void Disconnect();

  // Stop the TCP connection (if because of acceptable exceptions in
  // hardware-level, just retry)
  void Stop();
  // Stop the TCP connection but keep the event loop running (shared loops).
  void StopWithoutQuit();

  void SetConnectionCallback(const NormalCallback& cb) {
    ConnectionCallback_ = cb;
  }
  void SetMessageCallback(const MessageCallback& cb) { MessageCallback_ = cb; }
  void SetWriteCompleteCallback(const NormalCallback& cb) {
    WriteCompleteCallback_ = cb;
  }

  void SetRetryOn(bool on) { should_retry_ = on; }

 private:
  void DisconnectInLoop();
  void StopInLoop();
  void StopInLoopWithoutQuit();

  // Build a new TCP connection and insert it into the corresponding I/O thread
  void LaunchNewConnectionCallback(int socket_fd);

  // Event manager defined by users
  EventManager* event_manager_;

  // Connector for creating a new TCP connection in main thread
  std::shared_ptr<Connector> connector_;

  // Connection created
  Connecting* connection_;

  bool should_retry_;
  bool can_connect_;

  // Spin lock protecting the connection pointer
  MutexLock connection_mutex_;

  // Callback function which will be called after this TCP connection creating
  // and before this TCP connection destroying
  NormalCallback ConnectionCallback_;

  // Callback function which will be called after each reading
  MessageCallback MessageCallback_;

  // Callback function which will be called after each real writing
  NormalCallback WriteCompleteCallback_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_REACTOR_MANAGER_H_
