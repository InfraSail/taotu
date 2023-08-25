/**
 * @file echo.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "EchoServer" which is an simple echo server.
 * @date 2022-02-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "echo.h"

#include <functional>
#include <string>

EchoServer::EchoServer(const taotu::NetAddress& listen_address,
                       bool should_reuse_port, size_t io_thread_amount)
    : event_managers_(io_thread_amount, new taotu::EventManager),
      server_(std::make_unique<taotu::Server>(&event_managers_, listen_address,
                                              should_reuse_port)) {
  server_->SetMessageCallback([this](taotu::Connecting& connection,
                                     taotu::IoBuffer* io_buffer,
                                     taotu::TimePoint time_point) {
    this->OnMessageCallback(connection, io_buffer, time_point);
  });
}
EchoServer::~EchoServer() {
  size_t event_managers_size = event_managers_.size();
  for (size_t i = 0; i < event_managers_size; ++i) {
    delete event_managers_[i];
  }
  taotu::END_LOG();
}

void EchoServer::Start() { server_->Start(); }

void EchoServer::OnMessageCallback(taotu::Connecting& connection,
                                   taotu::IoBuffer* io_buffer,
                                   taotu::TimePoint time_point) {
  std::string message{io_buffer->RetrieveAllAsString()};
  taotu::LOG_DEBUG("Fd(%d) is receiving %u bytes at %lld.", connection.Fd(),
                   message.size(), time_point.GetMicroseconds());
  connection.Send(message);
}
