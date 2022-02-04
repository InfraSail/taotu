/**
 * @file reactor.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "reactor.h"

#include <stdlib.h>
#include <string.h>

#include "balancer.h"
#include "connecting.h"
#include "logger.h"

using namespace taotu;

static NetAddress GetLocalNetAddress(int socket_fd) {
  struct sockaddr_in6 local_addr;
  ::memset(&local_addr, 0, sizeof(local_addr));
  socklen_t addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getsockname(socket_fd, reinterpret_cast<struct sockaddr*>(&local_addr),
                    &addr_len) < 0) {
    LOG(logger::kError, "Fail to get local network info when accepting!!!");
  }
  return NetAddress(local_addr);
}

Reactor::Reactor(NetAddress& listen_address, int thread_amout)
    : acceptor_(listen_address, true), should_stop_(false) {
  if (acceptor_.Fd() >= 0 && !acceptor_.IsListening()) {
    acceptor_.Listen();
  } else {
    LOG(logger::kError, "Fail to init the acceptor!!!");
    ::exit(-1);
  }
  for (int i = 0; i < thread_amout; ++i) {
    event_managers_.emplace_back(new EventManager);
  }
  balancer_ = std::make_unique<Balancer>(&event_managers_);
}
Reactor::~Reactor() {
  int thread_amout = event_managers_.size();
  for (int i = 0; i < thread_amout; ++i) {
    delete event_managers_[i];
    event_managers_[i] = nullptr;
  }
  event_managers_.clear();
}

void Reactor::Loop() {
  // TODO:
  while (!should_stop_) {
    NetAddress peer_address;
    int socket_fd = acceptor_.Accept(&peer_address);
    if (socket_fd < 0 || socket_fd > kMaxEventAmount) {
      LOG(logger::kError, "Fail to accept a new connection request!!!");
      continue;
    }
    balancer_->PickOneEventManager()->InsertNewConnection(
        socket_fd, GetLocalNetAddress(socket_fd), peer_address);
  }
}
