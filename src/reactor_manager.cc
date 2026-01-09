/**
 * @file reactor_manager.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementations of class "ServerReactorManager" which manages reactors
 * in the server and class "ClientReactorManager" which manages the "Reactor" in
 * the client.
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "reactor_manager.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <functional>
#include <string>

#include "balancer.h"
#include "connecting.h"
#include "event_manager.h"
#include "logger.h"
#include "net_address.h"
#include "spin_lock.h"

namespace taotu {

namespace {
NetAddress GetLocalAddress(int socket_fd) {
  struct sockaddr_in6 local_addr;
  ::memset(&local_addr, 0, sizeof(local_addr));
  auto addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getsockname(socket_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                    &addr_len) < 0) {
    LOG_ERROR("Fail to get local network info when accepting!!!");
  }
  return NetAddress(local_addr);
}
NetAddress GetPeerAddress(int socket_fd) {
  struct sockaddr_in6 local_addr;
  ::memset(&local_addr, 0, sizeof(local_addr));
  auto addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getpeername(socket_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                    &addr_len) < 0) {
    LOG_ERROR("Fail to get local network info when accepting!!!");
  }
  return NetAddress(local_addr);
}
}  // namespace

ServerReactorManager::ServerReactorManager(EventManagers* event_managers,
                                           const NetAddress& listen_address,
                                           bool should_reuse_port)
    : event_managers_(event_managers),
      acceptor_(std::make_unique<Acceptor>((*event_managers_)[0]->GetPoller(),
                                           listen_address, should_reuse_port)) {
  if (acceptor_->Fd() >= 0 && !acceptor_->IsListening()) {
    acceptor_->Listen();
    acceptor_->RegisterNewConnectionCallback(
        [this](int socket_fd, const NetAddress& peer_address) {
          this->AcceptNewConnectionCallback(socket_fd, peer_address);
        });
  } else {
    LOG_ERROR("Fail to init the acceptor!!!");
    ::exit(-1);
  }
  for (size_t i = 0; i < event_managers->size(); ++i) {
    // "Initialize" "Reactor"s
    (*event_managers_)[i]->SetCreateConnectionCallback(
        [this](EventManager* event_manager, int fd,
               const NetAddress& server_address,
               const NetAddress& peer_address) -> Connecting* {
          return this->NewOneConnectingFromObjectPool(
              event_manager, fd, server_address, peer_address);
        });
    (*event_managers_)[i]->SetDestroyConnectionCallback(
        [this](Connecting* connecting_ptr) {
          this->DeleteOneConnectingFromObjectPool(connecting_ptr);
        });
  }
  balancer_ = std::make_unique<Balancer>(event_managers_, 0);
}
ServerReactorManager::~ServerReactorManager() {}

void ServerReactorManager::Loop() {
  size_t io_thread_amount = (*event_managers_).size();
  for (size_t i = 1; i < io_thread_amount; ++i) {
    // Start each event loop in the corresponding I/O thread
    (*event_managers_)[i]->Loop();
  }
  (*event_managers_)[0]->Work();
  // Do not start an event loop in main thread (mainly for accepting)
  // Like this:
  // `event_managers_[0]->Work();`
  // Let user do it by themselves
}

void ServerReactorManager::AcceptNewConnectionCallback(
    int socket_fd, const NetAddress& peer_address) {
  auto* event_manager = balancer_->PickOneEventManager();
  if (!event_manager) {
    LOG_ERROR("No EventManager available, drop connection fd(%d)", socket_fd);
    ::close(socket_fd);
    return;
  }
  NetAddress local_address = GetLocalAddress(socket_fd);
  event_manager->RunSoon(
      [this, event_manager, socket_fd, local_address, peer_address]() {
        auto new_connection = event_manager->InsertNewConnection(
            socket_fd, local_address,
            peer_address);  // Insert the new connection in its own I/O thread
        if (!new_connection) {
          return;
        }
        new_connection->RegisterOnConnectionCallback(ConnectionCallback_);
        new_connection->RegisterOnMessageCallback(MessageCallback_);
        new_connection->RegisterWriteCallback(WriteCompleteCallback_);
        new_connection->RegisterCloseCallback(CloseCallback_);
        new_connection
            ->OnEstablishing();  // Set the status flag on and start reading
      });
}

ClientReactorManager::ClientReactorManager(EventManager* event_manager,
                                           const NetAddress& server_address)
    : event_manager_(event_manager),
      connector_(std::make_shared<Connector>(event_manager_, server_address)),
      connection_(nullptr),
      should_retry_(false),
      can_connect_(true) {
  connector_->RegisterNewConnectionCallback(
      [this](int socket_fd) { this->LaunchNewConnectionCallback(socket_fd); });
}
ClientReactorManager::~ClientReactorManager() {
  LOG_DEBUG("Client is destroying.");
  {
    LockGuard lock_guard(connection_mutex_);
    connection_ = nullptr;
  }
  if (connector_) {
    connector_->Stop();
  }
}

void ClientReactorManager::Connect() {
  LOG_DEBUG("Connect to [ IP(%s) Port(%u) ].",
            connector_->GetServerAddress().GetIp().c_str(),
            connector_->GetServerAddress().GetPort());
  should_retry_ = false;  // No auto-reconnect for client mode to avoid loops.
  can_connect_ = true;
  connector_->Start();
}
void ClientReactorManager::Disconnect() {
  auto self = shared_from_this();
  event_manager_->RunSoon([self]() { self->DisconnectInLoop(); });
}
void ClientReactorManager::Stop() {
  auto self = shared_from_this();
  event_manager_->RunSoon([self]() { self->StopInLoop(); });
}
void ClientReactorManager::StopWithoutQuit() {
  auto self = shared_from_this();
  event_manager_->RunSoon([self]() { self->StopInLoopWithoutQuit(); });
}
void ClientReactorManager::DisconnectInLoop() {
  should_retry_ = false;
  can_connect_ = false;
  if (connector_) {
    connector_->Stop();
  }
  Connecting* connection_to_close = nullptr;
  {
    LockGuard lock_guard(connection_mutex_);
    if (connection_ != nullptr) {
      connection_to_close = connection_;
      connection_ = nullptr;
    }
  }
  if (connection_to_close) {
    connection_to_close->ForceClose();
  }
  event_manager_->Quit();
  event_manager_->WakeUp();
}
void ClientReactorManager::StopInLoop() {
  should_retry_ = false;
  can_connect_ = false;
  if (connector_) {
    connector_->Stop();
  }
  Connecting* connection_to_close = nullptr;
  {
    LockGuard lock_guard(connection_mutex_);
    if (connection_ != nullptr) {
      connection_to_close = connection_;
      connection_ = nullptr;
    }
  }
  if (connection_to_close) {
    connection_to_close->ForceClose();
  }
  event_manager_->Quit();
  event_manager_->WakeUp();
}

void ClientReactorManager::StopInLoopWithoutQuit() {
  should_retry_ = false;
  can_connect_ = false;
  if (connector_) {
    connector_->Stop();
  }
  Connecting* connection_to_close = nullptr;
  {
    LockGuard lock_guard(connection_mutex_);
    if (connection_ != nullptr) {
      connection_to_close = connection_;
      connection_ = nullptr;
    }
  }
  if (connection_to_close) {
    connection_to_close->ForceClose();
  }
}

void ClientReactorManager::LaunchNewConnectionCallback(int socket_fd) {
  NetAddress peer_address(GetPeerAddress(socket_fd));
  NetAddress local_address(GetLocalAddress(socket_fd));
  auto new_connection = event_manager_->InsertNewConnection(
      socket_fd, local_address, peer_address);
  if (!new_connection) {
    return;
  }
  new_connection->RegisterOnConnectionCallback(ConnectionCallback_);
  new_connection->RegisterOnMessageCallback(MessageCallback_);
  new_connection->RegisterWriteCallback(WriteCompleteCallback_);
  new_connection->RegisterCloseCallback([this](Connecting& connection) {
    {
      LockGuard lock_guard(connection_mutex_);
      connection_ = nullptr;
    }
    connection.ForceClose();
  });
  {
    LockGuard lock_guard(connection_mutex_);
    connection_ = new_connection;
  }
  new_connection->OnEstablishing();  // Set the status flag on and start reading
}

}  // namespace taotu
