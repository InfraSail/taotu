/**
 * @file reactor_manager.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
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
    LOG(logger::kError, "Fail to get local network info when accepting!!!");
  }
  return NetAddress(local_addr);
}
static NetAddress GetPeerAddress(int socket_fd) {
  struct sockaddr_in6 local_addr;
  ::memset(&local_addr, 0, sizeof(local_addr));
  auto addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getpeername(socket_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                    &addr_len) < 0) {
    LOG(logger::kError, "Fail to get local network info when accepting!!!");
  }
  return NetAddress(local_addr);
}

ServerReactorManager::ServerReactorManager(const NetAddress& listen_address,
                                           size_t io_thread_amount,
                                           bool should_reuse_port)
    : event_managers_(1, new EventManager),
      acceptor_(std::make_unique<Acceptor>(event_managers_[0]->GetPoller(),
                                           listen_address, should_reuse_port)) {
  if (acceptor_->Fd() >= 0 && !acceptor_->IsListening()) {
    acceptor_->Listen();
    acceptor_->RegisterNewConnectionCallback(
        [this](int socket_fd, const NetAddress& peer_address) {
          this->AcceptNewConnectionCallback(socket_fd, peer_address);
        });
  } else {
    LOG(logger::kError, "Fail to init the acceptor!!!");
    ::exit(-1);
  }
  for (size_t i = 0; i < io_thread_amount; ++i) {
    event_managers_.emplace_back(new EventManager);
  }
  balancer_ = std::make_unique<Balancer>(&event_managers_, 0);
}
ServerReactorManager::~ServerReactorManager() {
  size_t thread_amount = event_managers_.size();
  for (size_t i = 0; i < thread_amount; ++i) {
    delete event_managers_[i];
  }
}

void ServerReactorManager::Loop() {
  size_t io_thread_amount = event_managers_.size();
  for (size_t i = 1; i < io_thread_amount; ++i) {
    event_managers_[i]->Loop();
  }
  event_managers_[0]->Work();
}

void ServerReactorManager::AcceptNewConnectionCallback(
    int socket_fd, const NetAddress& peer_address) {
  auto new_connection = balancer_->PickOneEventManager()->InsertNewConnection(
      socket_fd, GetLocalAddress(socket_fd), peer_address);
  new_connection->RegisterOnConnectionCallback(ConnectionCallback_);
  new_connection->RegisterOnMessageCallback(MessageCallback_);
  new_connection->RegisterWriteCallback(WriteCompleteCallback_);
  new_connection->RegisterCloseCallback(CloseCallback_);
  new_connection->OnEstablishing();
  ConnectionCallback_(*new_connection);
}

ClientReactorManager::ClientReactorManager(const NetAddress& server_address,
                                           bool in_current_thread)
    : event_manager_(),
      connector_(std::make_unique<Connector>(&event_manager_, server_address)),
      connection_(nullptr),
      should_retry_(false),
      can_connect_(true),
      in_current_thread_(in_current_thread) {
  connector_->RegisterNewConnectionCallback(
      [this](int socket_fd) { this->LaunchNewConnectionCallback(socket_fd); });
}
ClientReactorManager::~ClientReactorManager() {
  LOG(logger::kDebug, "Client is destroying.");
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
    connector_->Stop();
  }
}

void ClientReactorManager::Connect() {
  LOG(logger::kDebug, "Connect to IP(%s) Port(%u)",
      connector_->GetServerAddress().GetIp().c_str(),
      connector_->GetServerAddress().GetPort());
  can_connect_ = true;
  connector_->Start();
  if (in_current_thread_) {
    event_manager_.Work();
  } else {
    event_manager_.Loop();
  }
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
  connector_->Stop();
}

void ClientReactorManager::LaunchNewConnectionCallback(int socket_fd) {
  NetAddress peer_address(GetPeerAddress(socket_fd));
  NetAddress local_address(GetLocalAddress(socket_fd));
  auto new_connection = event_manager_.InsertNewConnection(
      socket_fd, local_address, peer_address);
  new_connection->RegisterOnConnectionCallback(ConnectionCallback_);
  new_connection->RegisterOnMessageCallback(MessageCallback_);
  new_connection->RegisterWriteCallback(WriteCompleteCallback_);
  new_connection->RegisterCloseCallback(std::bind(
      [this](Connecting& connection) {
        {
          LockGuard lock_guard(connection_mutex_);
          connection_ = nullptr;
        }
        connection.ForceClose();
        if (this->should_retry_ && this->can_connect_) {
          LOG(logger::kDebug, "Reconnect to [ Ip(%s), Port(%u) ].",
              this->connector_->GetServerAddress().GetIp().c_str(),
              this->connector_->GetServerAddress().GetPort());
          this->connector_->Restart();
        }
      },
      std::placeholders::_1));
  new_connection->OnEstablishing();
  {
    LockGuard lock_guard(connection_mutex_);
    connection_ = new_connection;
  }
}
