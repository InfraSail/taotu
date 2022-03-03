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
                             bool should_reuse_port)
    : server_(
          std::make_unique<taotu::Server>(listen_address, should_reuse_port)) {
  server_->SetConnectionCallback(std::bind(&DiscardServer::OnConnectionCallback,
                                           this, std::placeholders::_1));
  server_->SetMessageCallback(
      std::bind(&DiscardServer::OnMessageCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
}

void DiscardServer::Start() { server_->Start(); }

void DiscardServer::OnConnectionCallback(taotu::Connecting& connection) {
  taotu::LOG(taotu::logger::kDebug,
             "EchoServer - (Ip(" + connection.GetLocalNetAddress().GetIp() +
                 "), Port(" +
                 std::to_string(connection.GetLocalNetAddress().GetPort()) +
                 ")) -> (Ip(" + connection.GetPeerNetAddress().GetIp() +
                 "), Port(" +
                 std::to_string(connection.GetPeerNetAddress().GetPort()) +
                 ")) - " + (connection.IsConnected() ? "UP." : "Down."));
}
void DiscardServer::OnMessageCallback(taotu::Connecting& connection,
                                      taotu::IoBuffer* io_buffer,
                                      taotu::TimePoint time_point) {
  std::string message{io_buffer->RetrieveAllAsString()};
  taotu::LOG(taotu::logger::kDebug,
             "Fd(" + std::to_string(connection.Fd()) + ") is echoing " +
                 std::to_string(message.size()) + " bytes(" +
                 message.substr(0, message.size() - 1) + ") received at " +
                 std::to_string(time_point.GetMicroseconds()) + ".");
  ::printf("%s", message.c_str());
}
