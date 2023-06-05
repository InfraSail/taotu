/**
 * @file pingpong_client.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "PingpongClient" which is a pingpong total
 * client and class "Session" which manage one connection to the server.
 * @date 2022-03-28
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "pingpong_client.h"

#include <stdio.h>

#include <utility>

#include "../../src/balancer.h"
#include "../../src/logger.h"

PingpongClient::PingpongClient(const taotu::NetAddress& server_address,
                               size_t block_size, size_t session_count,
                               int timeout, size_t thread_count)
    : event_managers_(thread_count + 1),
      session_count_(session_count),
      timeout_(timeout),
      message_() {
  thread_count += 2;
  event_managers_.push_back(nullptr);
  event_managers_[0] = nullptr;
  event_managers_[1] = new taotu::EventManager;
  event_managers_[1]->RunAfter(timeout_ * 1000 * 1000,
                               [this]() { this->DoWithTimeout(); });
  conn_num_.store(0);
  for (size_t i = 2; i < thread_count; ++i) {
    event_managers_[i] = new taotu::EventManager;
    event_managers_[i]->Loop();
  }
  for (size_t i = 1; i <= block_size; ++i) {
    message_.emplace_back(static_cast<char>(i % 128));
  }
  balancer_ = std::make_unique<taotu::Balancer>(&event_managers_, 0);
  for (size_t i = 0; i < session_count_; ++i) {
    sessions_.emplace_back(std::make_unique<Session>(
        balancer_->PickOneEventManager(), server_address, shared_from_this()));
    sessions_[i]->Start();
  }
}
PingpongClient::~PingpongClient() {
  size_t thread_count = event_managers_.size();
  for (size_t i = 1; i < thread_count; ++i) {
    auto& event_manager = event_managers_[i];
    event_manager->RunSoon([&event_manager]() { event_manager->Quit(); });
  }
  for (size_t i = thread_count - 1; i > 0; --i) {
    delete event_managers_[i];
  }
  taotu::END_LOG();
}

void PingpongClient::Start() {
  if (event_managers_.size() > 1) {
    event_managers_[1]->Work();
  }
}

void PingpongClient::OnConnecting() {
  if (conn_num_.fetch_add(1) + 1 == session_count_) {
    taotu::LOG_WARN("All connected!");
  }
}

void PingpongClient::OnDisconnecting(taotu::Connecting& connection) {
  if (conn_num_.fetch_sub(1) - 1 == 0) {
    int64_t total_bytes_read = 0;
    int64_t total_messages_read = 0;
    for (const auto& session : sessions_) {
      total_bytes_read += session->GetBytesRead();
      total_messages_read += session->GetMessagesRead();
    }
    ::printf(
        "Totally,\n%ldbytes read\nand %ldmessages read,\nthe average message "
        "size is %lf,\nand the throughput is %lfMiB/s.\n",
        total_bytes_read, total_messages_read,
        static_cast<double>(total_bytes_read) /
            static_cast<double>(total_messages_read),
        static_cast<double>(total_bytes_read) / (timeout_ * 1024 * 1024));
    event_managers_[1]->RunSoon([this] { this->event_managers_[1]->Quit(); });
  }
}

void PingpongClient::DoWithTimeout() {
  taotu::LOG_WARN("All stopped!");
  for (auto& session : sessions_) {
    session->Stop();
  }
}

Session::Session(taotu::EventManager* event_manager,
                 const taotu::NetAddress& server_address,
                 std::shared_ptr<PingpongClient> master_client)
    : client_(event_manager, server_address, true),
      master_client_(master_client),
      bytes_read_(0),
      messages_read_(0) {
  client_.SetConnectionCallback([this](taotu::Connecting& connection) {
    this->OnConnectionCallback(connection);
  });
  client_.SetMessageCallback([this](taotu::Connecting& connection,
                                    taotu::IoBuffer* io_buffer,
                                    taotu::TimePoint time_point) {
    this->OnMessageCallback(connection, io_buffer, std::move(time_point));
  });
}

void Session::Start() { client_.Connect(); }

void Session::Stop() { client_.Disconnect(); }

void Session::OnConnectionCallback(taotu::Connecting& connection) {
  if (connection.IsConnected()) {
    connection.SetTcpNoDelay(true);
    std::shared_ptr<PingpongClient> master_client(master_client_.lock());
    if (master_client) {
      const auto& message = master_client->GetMessage();
      connection.Send(&(*(message.begin())), message.size());
      master_client->OnConnecting();
    }
  } else {
    std::shared_ptr<PingpongClient> master_client(master_client_.lock());
    if (master_client) {
      master_client->OnDisconnecting(connection);
    }
  }
}

void Session::OnMessageCallback(taotu::Connecting& connection,
                                taotu::IoBuffer* io_buffer, taotu::TimePoint) {
  ++messages_read_;
  bytes_read_ += static_cast<int64_t>(io_buffer->GetReadableBytes());
  connection.Send(io_buffer);
}
