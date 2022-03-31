/**
 * @file chat_server.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-21
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "chat_server.h"

#include <stdio.h>

ChatServer::ChatServer(const taotu::NetAddress& listen_address,
                       bool should_reuse_port, size_t io_thread_amount,
                       size_t calculation_thread_amount)
    : event_manager_(new taotu::EventManager),
      server_(std::make_unique<taotu::Server>(
          event_manager_, listen_address, should_reuse_port, io_thread_amount,
          calculation_thread_amount)),
      codec_([this](taotu::Connecting& connection, const std::string& message,
                    taotu::TimePoint time_point) {
        this->OnCodecMessage(connection, message, time_point);
      }) {
  server_->SetConnectionCallback([this](taotu::Connecting& connection) {
    this->OnConnection(connection);
  });
  server_->SetMessageCallback([this](taotu::Connecting& connection,
                                     taotu::IoBuffer* io_buffer,
                                     taotu::TimePoint time_point) {
    this->codec_.OnMessage(connection, io_buffer, time_point);
  });
}

void ChatServer::Start() {
  server_->Start();
  event_manager_->Work();
}

void ChatServer::OnConnection(taotu::Connecting& connection) {
  taotu::LOG(
      taotu::logger::kDebug,
      "Create a new connection with fd(%d) between local net address (IP(%s), "
      "Port(%s)) and peer net address (IP(%s), Port(%s)).",
      connection.Fd(), connection.GetLocalNetAddress().GetIp().c_str(),
      std::to_string(connection.GetLocalNetAddress().GetPort()).c_str(),
      connection.GetPeerNetAddress().GetIp().c_str(),
      std::to_string(connection.GetPeerNetAddress().GetPort()).c_str());
  taotu::LockGuard lock_guard(connections_lock_);
  if (connection.IsConnected()) {
    connections_.insert(&connection);
  } else {
    connections_.erase(&connection);
  }
}
void ChatServer::OnCodecMessage(taotu::Connecting& connection,
                                const std::string& message, taotu::TimePoint) {
  ::printf("<<< %s\n", message.c_str());
  taotu::LockGuard lock_guard(connections_lock_);
  int conn_fd = connection.Fd();
  for (ConnectionSet::iterator itr = connections_.begin();
       itr != connections_.end(); ++itr) {
    if (conn_fd != (*itr)->Fd()) {
      codec_.Send(*itr, message);
    }
  }
}
