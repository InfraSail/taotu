/**
 * @file pingpong_server.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "PingpongServer" which is a pingpong server.
 * @date 2022-03-28
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "pingpong_server.h"

PingpongServer::PingpongServer(const taotu::NetAddress& listen_address,
                               bool should_reuse_port, size_t io_thread_amount,
                               size_t calculation_thread_amount)
    : event_manager_(new taotu::EventManager),
      server_(std::make_unique<taotu::Server>(
          event_manager_, listen_address, should_reuse_port, io_thread_amount,
          calculation_thread_amount)) {
  server_->SetConnectionCallback([this](taotu::Connecting& connection) {
    this->OnConnectionCallback(connection);
  });
  server_->SetMessageCallback([this](taotu::Connecting& connection,
                                     taotu::IoBuffer* io_buffer,
                                     taotu::TimePoint time_point) {
    this->OnMessageCallback(connection, io_buffer, time_point);
  });
}
PingpongServer::~PingpongServer() {
  delete event_manager_;
  taotu::END_LOG();
}

void PingpongServer::Start() {
  server_->Start();
  event_manager_->Work();
}

void PingpongServer::OnConnectionCallback(taotu::Connecting& connection) {
  if (connection.IsConnected()) {
    connection.SetTcpNoDelay(true);
  }
}
void PingpongServer::OnMessageCallback(taotu::Connecting& connection,
                                       taotu::IoBuffer* io_buffer,
                                       taotu::TimePoint time_point) {
  connection.Send(io_buffer);
}
