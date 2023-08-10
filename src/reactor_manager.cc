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

#include <functional>
#include <string>

#include "balancer.h"
#include "connecting.h"
#include "event_manager.h"
#include "logger.h"
#include "net_address.h"
#include "spin_lock.h"

using namespace taotu;

static NetAddress GetLocalAddress(int socket_fd) {
  struct sockaddr_in6 local_addr;
  ::memset(&local_addr, 0, sizeof(local_addr));
  auto addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getsockname(socket_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                    &addr_len) < 0) {
    LOG_ERROR("Fail to get local network info when accepting!!!");
  }
  return NetAddress(local_addr);
}
static NetAddress GetPeerAddress(int socket_fd) {
  struct sockaddr_in6 local_addr;
  ::memset(&local_addr, 0, sizeof(local_addr));
  auto addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getpeername(socket_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                    &addr_len) < 0) {
    LOG_ERROR("Fail to get local network info when accepting!!!");
  }
  return NetAddress(local_addr);
}

ServerReactorManager::ServerReactorManager(EventManagers* event_managers,
                                           const NetAddress& listen_address,
                                           size_t io_thread_amount,
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
  // TODO: event_managers_[0] = event_managers;
  for (size_t i = 0; i < io_thread_amount; ++i) {
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
ServerReactorManager::~ServerReactorManager() {
  // size_t thread_amount = event_managers_.size();
  // for (size_t i = 1; i < thread_amount; ++i) {
  //   delete event_managers_[i];
  // }
}

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
  auto new_connection = balancer_->PickOneEventManager()->InsertNewConnection(
      socket_fd, GetLocalAddress(socket_fd),
      peer_address);  // Pick a "Reactor" with the lowest load and insert the
                      // new connection created just now into it
  new_connection->RegisterOnConnectionCallback(ConnectionCallback_);
  new_connection->RegisterOnMessageCallback(MessageCallback_);
  new_connection->RegisterWriteCallback(WriteCompleteCallback_);
  new_connection->RegisterCloseCallback(CloseCallback_);
  new_connection->OnEstablishing();  // Set the status flag on and start reading
}

ClientReactorManager::ClientReactorManager(EventManager* event_manager,
                                           const NetAddress& server_address)
    : event_manager_(event_manager),
      connector_(event_manager_, server_address),
      connection_(nullptr),
      should_retry_(false),
      can_connect_(true) {
  connector_.RegisterNewConnectionCallback(
      [this](int socket_fd) { this->LaunchNewConnectionCallback(socket_fd); });
}
ClientReactorManager::~ClientReactorManager() {
  LOG_DEBUG("Client is destroying.");
  Connecting* connection;
  {
    LockGuard lock_guard(connection_mutex_);
    connection = connection_;
  }
  if (connection != nullptr) {
    connection->RegisterCloseCallback(
        [](Connecting& connection) { connection.ForceClose(); });
    connection->ForceClose();
  } else {
    connector_.Stop();
  }
}

void ClientReactorManager::Connect() {
  LOG_DEBUG("Connect to [ IP(%s) Port(%u) ].",
            connector_.GetServerAddress().GetIp().c_str(),
            connector_.GetServerAddress().GetPort());
  can_connect_ = true;
  connector_.Start();
}
void ClientReactorManager::Disconnect() {
  can_connect_ = false;
  {
    LockGuard lock_guard(connection_mutex_);
    if (connection_ != nullptr) {
      connection_->ShutDownWrite();
    }
  }
}
void ClientReactorManager::Stop() {
  can_connect_ = false;
  connector_.Stop();
}

void ClientReactorManager::LaunchNewConnectionCallback(int socket_fd) {
  NetAddress peer_address(GetPeerAddress(socket_fd));
  NetAddress local_address(GetLocalAddress(socket_fd));
  auto new_connection = event_manager_->InsertNewConnection(
      socket_fd, local_address, peer_address);
  new_connection->RegisterOnConnectionCallback(ConnectionCallback_);
  new_connection->RegisterOnMessageCallback(MessageCallback_);
  new_connection->RegisterWriteCallback(WriteCompleteCallback_);
  new_connection->RegisterCloseCallback([this](Connecting& connection) {
    {
      LockGuard lock_guard(connection_mutex_);
      connection_ = nullptr;
    }
    connection.ForceClose();
    if (this->should_retry_ && this->can_connect_) {
      LOG_DEBUG("Reconnect to [ IP(%s), Port(%u) ].",
                this->connector_.GetServerAddress().GetIp().c_str(),
                this->connector_.GetServerAddress().GetPort());
      this->connector_.Restart();
    }
  });
  {
    LockGuard lock_guard(connection_mutex_);
    connection_ = new_connection;
  }
  new_connection->OnEstablishing();  // Set the status flag on and start reading
}
