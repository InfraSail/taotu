/**
 * @file discard.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
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
    : server_(std::make_unique<taotu::Server>(listen_address, should_reuse_port,
                                              io_thread_amount,
                                              calculation_thread_amount)) {
  server_->SetMessageCallback([this](taotu::Connecting& connection,
                                     taotu::IoBuffer* io_buffer,
                                     taotu::TimePoint time_point) {
    this->OnMessageCallback(connection, io_buffer, time_point);
  });
}

void DiscardServer::Start() { server_->Start(); }

void DiscardServer::OnMessageCallback(taotu::Connecting& connection,
                                      taotu::IoBuffer* io_buffer,
                                      taotu::TimePoint time_point) {
  std::string message{io_buffer->RetrieveAllAsString()};
  taotu::LOG(taotu::logger::kDebug, "Fd(%d) is receiving %u bytes(%s) at %lld.",
             connection.Fd(), message.size(),
             message.substr(0, message.size() - 1).c_str(),
             time_point.GetMicroseconds());
  ::printf("%s", message.c_str());
  connection.Send("");
}
