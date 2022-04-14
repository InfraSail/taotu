/**
 * @file pingpong_client.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
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
    : event_managers_(++thread_count),
      session_count_(session_count),
      timeout_(timeout) {
  event_managers_[0] = new taotu::EventManager;
  event_managers_[0]->RunAfter(timeout_ * 1000 * 1000,
                               [this]() { this->DoWithTimeout(); });
  for (size_t i = 1; i < thread_count; ++i) {
    event_managers_[i] = new taotu::EventManager;
    event_managers_[i]->Loop();
  }
  for (size_t i = 0; i < block_size; ++i) {
    message_.push_back(static_cast<char>(i % 128));
  }
  balancer_ = std::make_unique<taotu::Balancer>(&event_managers_, 0);
  for (size_t i = 0; i < session_count_; ++i) {
    sessions_.emplace_back(std::make_unique<Session>(
        balancer_->PickOneEventManager(), server_address, shared_from_this()));
    sessions_[i]->Start();
  }
}
PingpongClient::~PingpongClient() {
  for (auto& event_manager : event_managers_) {
    delete event_manager;
  }
}

void PingpongClient::Start() {
  if (!event_managers_.empty()) {
    event_managers_[0]->Work();
  }
}

void PingpongClient::OnConnecting() {
  if (conn_num_.load() == session_count_) {
    taotu::LOG(taotu::logger::kWarn, "All connected!");
  }
}

void PingpongClient::OnDisconnecting(taotu::Connecting& connection) {
  if (conn_num_.load() == 0) {
    int64_t total_bytes_read = 0;
    int64_t total_messages_read = 0;
    for (const auto& session : sessions_) {
      total_bytes_read += session->GetBytesRead();
      total_messages_read += session->GetMessagesRead();
    }
    taotu::LOG(
        taotu::logger::kWarn,
        "All disconnected: totally %llubytes read and %llumessages read, the "
        "average message size is %lf and the throughput is %lfMiB/s!",
        total_bytes_read, total_messages_read,
        static_cast<double>(total_bytes_read) /
            static_cast<double>(total_messages_read),
        static_cast<double>(total_bytes_read) / (timeout_ * 1024 * 1024));
    auto& master_event_manager = connection.GetEventManager();
    master_event_manager.RunSoon([&]() { master_event_manager.Quit(); });
  }
}

void PingpongClient::DoWithTimeout() {
  taotu::LOG(taotu::logger::kWarn, "All stopped!");
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
    connection.Send(master_client_->GetMessage());
    master_client_->OnConnecting();
  } else {
    master_client_->OnDisconnecting(connection);
  }
}

void Session::OnMessageCallback(taotu::Connecting& connection,
                                taotu::IoBuffer* io_buffer, taotu::TimePoint) {
  ++messages_read_;
  bytes_read_ += io_buffer->GetReadableBytes();
  connection.Send(io_buffer);
}
