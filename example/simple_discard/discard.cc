/**
 * @file discard.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "DiscardServer" which is a simple discard
 * server.
 * @date 2022-03-01
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "discard.h"

#include <stdio.h>

#include <string>

DiscardServer::DiscardServer(const taotu::NetAddress& listen_address,
                             bool should_reuse_port, size_t io_thread_amount,
                             size_t calculation_thread_amount)
    : event_managers_(io_thread_amount, new taotu::EventManager),
      server_(std::make_unique<taotu::Server>(
          &event_managers_, listen_address, should_reuse_port, io_thread_amount,
          calculation_thread_amount)) {
  server_->SetMessageCallback([this](taotu::Connecting& connection,
                                     taotu::IoBuffer* io_buffer,
                                     taotu::TimePoint time_point) {
    this->OnMessageCallback(connection, io_buffer, time_point);
  });
}
DiscardServer::~DiscardServer() {
  size_t event_managers_size = event_managers_.size();
  for (size_t i = 0; i < event_managers_size; ++i) {
    delete event_managers_[i];
  }
  taotu::END_LOG();
}

void DiscardServer::Start() { server_->Start(); }

void DiscardServer::OnMessageCallback(taotu::Connecting& connection,
                                      taotu::IoBuffer* io_buffer,
                                      taotu::TimePoint time_point) {
  std::string message{io_buffer->RetrieveAllAsString()};
  taotu::LOG_DEBUG("Fd(%d) is receiving %u bytes(%s) at %lld.", connection.Fd(),
                   message.size(),
                   message.substr(0, message.size() - 1).c_str(),
                   time_point.GetMicroseconds());
  ::printf("%s", message.c_str());
  connection.Send("");
}
