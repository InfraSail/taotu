/**
 * @file chat_server.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "ChatServer" which is a chat server.
 * @date 2022-03-21
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "chat_server.h"

#include <stdio.h>

ChatServer::ChatServer(const taotu::NetAddress& listen_address,
                       bool should_reuse_port, size_t io_thread_amount)
    : event_managers_(io_thread_amount, new taotu::EventManager),
      server_(std::make_unique<taotu::Server>(&event_managers_, listen_address,
                                              should_reuse_port)),
      codec_([this](taotu::Connecting& connection, const std::string& message,
                    taotu::TimePoint time_point) {
        this->OnCodecMessage(connection, message, time_point);
      }) {
  server_->SetConnectionCallback([this](taotu::Connecting& connection) {
    this->OnConnectionCallback(connection);
  });
  server_->SetMessageCallback([this](taotu::Connecting& connection,
                                     taotu::IoBuffer* io_buffer,
                                     taotu::TimePoint time_point) {
    this->codec_.OnMessage(connection, io_buffer, time_point);
  });
}
ChatServer::~ChatServer() {
  size_t event_managers_size = event_managers_.size();
  for (size_t i = 0; i < event_managers_size; ++i) {
    delete event_managers_[i];
  }
  taotu::END_LOG();
}

void ChatServer::Start() { server_->Start(); }

void ChatServer::OnConnectionCallback(taotu::Connecting& connection) {
  taotu::LOG_DEBUG(
      "Create a new connection with fd(%d) between local net address [ IP(%s), "
      "Port(%s) ] and peer net address [ IP(%s), Port(%s) ].",
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

  // Send the message to all other clients
  for (ConnectionSet::iterator itr = connections_.begin();
       itr != connections_.end(); ++itr) {
    if (conn_fd != (*itr)->Fd()) {
      codec_.Send(*itr, message);
    }
  }
}
