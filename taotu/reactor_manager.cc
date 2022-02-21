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
#include <memory>

#include "balancer.h"
#include "connecting.h"
#include "logger.h"
#include "net_address.h"

using namespace taotu;

static NetAddress GetLocalAddress(int socket_fd) {
  struct sockaddr_in6 local_addr;
  ::memset(&local_addr, 0, sizeof(local_addr));
  socklen_t addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getsockname(socket_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                    &addr_len) < 0) {
    LOG(logger::kError, "Fail to get local network info when accepting!!!");
  }
  return NetAddress(local_addr);
}
static NetAddress GetPeerAddress(int socket_fd) {
  struct sockaddr_in6 local_addr;
  ::memset(&local_addr, 0, sizeof(local_addr));
  socklen_t addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getpeername(socket_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                    &addr_len) < 0) {
    LOG(logger::kError, "Fail to get local network info when accepting!!!");
  }
  return NetAddress(local_addr);
}

ServerReactorManager::ServerReactorManager(const NetAddress& listen_address,
                                           int io_thread_amount,
                                           bool should_reuse_port)
    : event_managers_(1, std::make_unique<EventManager>()),
      acceptor_(std::make_unique<Acceptor>(event_managers_[0]->GetPoller(),
                                           listen_address, should_reuse_port)) {
  if (acceptor_->Fd() >= 0 && !acceptor_->IsListening()) {
    acceptor_->Listen();
    acceptor_->RegisterNewConnectionCallback(
        std::bind(&ServerReactorManager::AcceptNewConnectionCallback, this,
                  std::placeholders::_1, std::placeholders::_2));
  } else {
    LOG(logger::kError, "Fail to init the acceptor!!!");
    ::exit(-1);
  }
  for (int i = 1; i < io_thread_amount; ++i) {
    event_managers_.emplace_back(std::make_unique<EventManager>());
  }
  balancer_ = std::make_unique<Balancer>(&event_managers_);
}
ServerReactorManager::~ServerReactorManager() {}

void ServerReactorManager::Loop() {
  int io_thread_amount = event_managers_.size();
  for (int i = 1; i < io_thread_amount; ++i) {
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
}

ClientReactorManager::ClientReactorManager(const NetAddress& server_address)
    : event_manager_(),
      connector_(std::make_unique<Connector>(&event_manager_, server_address)) {
}
ClientReactorManager::~ClientReactorManager() {}

void ClientReactorManager::LaunchNewConnectionCallback(int socket_fd) {
  NetAddress peer_address(GetPeerAddress(socket_fd));
  NetAddress local_address(GetLocalAddress(socket_fd));
}
