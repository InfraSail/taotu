/**
 * @file chat_client.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-21
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "chat_client.h"

#include <stdio.h>

ChatClient::ChatClient(const taotu::NetAddress& connect_address)
    : event_manager_(std::make_shared<taotu::EventManager>()),
      client_(std::make_unique<taotu::Client>(event_manager_.get(),
                                              connect_address, true)),
      codec_([this](taotu::Connecting& connection, const std::string& message,
                    taotu::TimePoint time_point) {
        this->OnCodecMessage(connection, message, time_point);
      }) {
  event_manager_->Loop();
  client_->SetConnectionCallback([this](taotu::Connecting& connection) {
    this->OnConnection(connection);
  });
  client_->SetMessageCallback([this](taotu::Connecting& connection,
                                     taotu::IoBuffer* io_buffer,
                                     taotu::TimePoint time_point) {
    this->codec_.OnMessage(connection, io_buffer, time_point);
  });
}

void ChatClient::Connect() { client_->Connect(); }
void ChatClient::Disconnect() { client_->Disconnect(); }

void ChatClient::Write(const std::string& message) {
  taotu::LockGuard lock_guard(connection_lock_);
  if (connection_ != nullptr) {
    codec_.Send(connection_, message);
  }
}

void ChatClient::OnConnection(taotu::Connecting& connection) {
  taotu::LOG(
      taotu::logger::kDebug,
      "Create a new connection with fd(%d) between local net address (IP(%s), "
      "Port(%s)) and peer net address (IP(%s), Port(%s)).",
      connection.Fd(), connection.GetLocalNetAddress().GetIp().c_str(),
      std::to_string(connection.GetLocalNetAddress().GetPort()).c_str(),
      connection.GetPeerNetAddress().GetIp().c_str(),
      std::to_string(connection.GetPeerNetAddress().GetPort()).c_str());
  taotu::LockGuard lock_guard(connection_lock_);
  if (connection.IsConnected()) {
    connection_ = &connection;
  } else {
    connection_ = nullptr;
  }
}
void ChatClient::OnCodecMessage(taotu::Connecting&, const std::string& message,
                                taotu::TimePoint) {
  ::printf("<<< %s\n", message.c_str());
}
