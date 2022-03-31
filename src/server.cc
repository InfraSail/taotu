/**
 * @file server.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Server" which is the encapsulation of the
 * server.
 * @date 2022-01-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "server.h"

#include <string>

// #include "logger.h"
#include "reactor_manager.h"

using namespace taotu;

Server::Server(EventManager* event_manager, const NetAddress& listen_address,
               bool should_reuse_port, size_t io_thread_amount,
               size_t calculation_thread_amount)
    : reactor_manager_(event_manager, listen_address, io_thread_amount,
                       should_reuse_port),
      thread_pool_(calculation_thread_amount),
      is_started_(false) {
  reactor_manager_.SetConnectionCallback([this](Connecting& connection) {
    this->DefaultOnConnectionCallback(connection);
  });
  reactor_manager_.SetMessageCallback([this](Connecting& connection,
                                             IoBuffer* io_buffer,
                                             TimePoint time_point) {
    this->DefaultOnMessageCallback(connection, io_buffer, time_point);
  });
  reactor_manager_.SetCloseCallback(
      [this](Connecting& connection) { this->RemoveConnection(connection); });
}

void Server::SetConnectionCallback(const std::function<void(Connecting&)>& cb) {
  reactor_manager_.SetConnectionCallback(cb);
}
void Server::SetMessageCallback(
    const std::function<void(Connecting&, IoBuffer*, TimePoint)>& cb) {
  reactor_manager_.SetMessageCallback(cb);
}
void Server::SetWriteCompleteCallback(
    const std::function<void(Connecting&)>& cb) {
  reactor_manager_.SetWriteCompleteCallback(cb);
}
void Server::SetCloseCallback(const std::function<void(Connecting&)>& cb) {
  reactor_manager_.SetCloseCallback(cb);
}

void Server::Start() {
  if (!is_started_.load()) {
    is_started_.store(true);
    reactor_manager_.Loop();
  }
}

void Server::RemoveConnection(Connecting& connection) {
  LOG(logger::kDebug, "The connection with fd(%d) is being removed.",
      connection.Fd());
  connection.ForceClose();
}

void Server::DefaultOnConnectionCallback(Connecting& connection) {
  LOG(logger::kDebug,
      "A new connection with fd(%d) on local IP(%s) Port(%u) and peer IP(%s) "
      "Port(%u) is %s now.",
      connection.Fd(), connection.GetLocalNetAddress().GetIp().c_str(),
      connection.GetLocalNetAddress().GetPort(),
      connection.GetPeerNetAddress().GetIp().c_str(),
      connection.GetPeerNetAddress().GetPort(),
      (connection.IsConnected() ? "created" : "closed"));
}
void Server::DefaultOnMessageCallback(Connecting& connection,
                                      IoBuffer* io_buffer,
                                      TimePoint time_point) {
  io_buffer->RefreshRW();
}
