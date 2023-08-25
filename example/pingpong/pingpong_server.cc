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
                               bool should_reuse_port, size_t io_thread_amount)
    : event_managers_(io_thread_amount, new taotu::EventManager),
      server_(std::make_unique<taotu::Server>(&event_managers_, listen_address,
                                              should_reuse_port)) {
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
  size_t event_managers_size = event_managers_.size();
  for (size_t i = 0; i < event_managers_size; ++i) {
    delete event_managers_[i];
  }
  taotu::END_LOG();
}

void PingpongServer::Start() { server_->Start(); }

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
