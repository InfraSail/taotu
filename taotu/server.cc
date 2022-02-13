/**
 * @file server.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-01-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "server.h"

#include <functional>
#include <string>

#include "logger.h"
#include "reactor_manager.h"

using namespace taotu;

Server::Server(const NetAddress& listen_address, int io_thread_amount,
               bool should_reuse_port)
    : reactor_manager_(std::make_unique<ReactorManager>(
          listen_address, io_thread_amount, should_reuse_port)),
      is_started_(false) {
  reactor_manager_->SetConnectionCallback(std::bind(
      &Server::DefaultOnConnectionCallback, this, std::placeholders::_1));
  reactor_manager_->SetMessageCallback(
      std::bind(&Server::DefaultOnMessageCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
}
Server::~Server() {}

void Server::Start() {
  if (!is_started_.load()) {
    // TODO: the thread pool
    reactor_manager_->Loop();
  }
}

void Server::DefaultOnConnectionCallback(Connecting& connection) {
  LOG(logger::kDebug,
      "A new connection with fd(" + std::to_string(connection.Fd()) +
          ") on local IP(" + connection.GetLocalNetAddress().GetIp() +
          ") Port(" +
          std::to_string(connection.GetLocalNetAddress().GetPort()) +
          ") and peer IP(" + connection.GetPeerNetAddress().GetIp() +
          ") Port(" + std::to_string(connection.GetPeerNetAddress().GetPort()) +
          ").");
}
void Server::DefaultOnMessageCallback(Connecting& connection,
                                      IoBuffer* io_buffer,
                                      TimePoint time_point) {
  io_buffer->RefreshRW();
}
