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

#include "../../src/balancer.h"
#include "../../src/logger.h"

PingpongClient::PingpongClient(const taotu::NetAddress& server_address,
                               int block_size, int session_count, int timeout,
                               int thread_count)
    : event_manager_(std::make_unique<taotu::EventManager>()),
      event_managers_(++thread_count),
      session_count_(session_count),
      timeout_(timeout) {
  event_manager_->RunAfter(timeout_ * 1000 * 1000,
                           [this]() { this->DoWithTimeout(); });
  for (int i = 1; i < thread_count; ++i) {
    event_managers_[i] = new taotu::EventManager;
    event_managers_[i]->Loop();
  }
  for (int i = 0; i < block_size; ++i) {
    message_.push_back(static_cast<char>(i % 128));
  }
  balancer_ = std::make_unique<taotu::Balancer>(&event_managers_, 0);
  for (int i = 0; i < session_count_; ++i) {
    char buf[32];
    ::snprintf(buf, sizeof(buf), "C%05d", i);
    sessions_.push_back(std::make_unique<Session>(
        balancer_->PickOneEventManager(), server_address, this));
    sessions_.back()->Start();
  }
}
PingpongClient::~PingpongClient() {
  size_t thread_count = event_managers_.size();
  for (size_t i = 1; i < thread_count; ++i) {
    delete event_managers_[i];
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
    connection.GetEventManager().Quit();
  }
}

void PingpongClient::DoWithTimeout() {
  taotu::LOG(taotu::logger::kWarn, "");
  for (auto& session : sessions_) {
    session->Stop();
  }
}

Session::Session(taotu::EventManager* event_manager,
                 const taotu::NetAddress& server_address,
                 PingpongClient* client)
    : client_(event_manager, server_address, true),
      master_(client),
      bytes_read_(0),
      messages_read_(0) {
  client_.SetConnectionCallback([this](taotu::Connecting& connection) {
    this->OnConnectionCallback(connection);
  });
  client_.SetMessageCallback([this](taotu::Connecting& connection,
                                    taotu::IoBuffer* io_buffer,
                                    taotu::TimePoint time_point) {
    this->OnMessageCallback(connection, io_buffer, time_point);
  });
}

void Session::Start() { client_.Connect(); }

void Session::Stop() { client_.Disconnect(); }

void Session::OnConnectionCallback(taotu::Connecting& connection) {
  if (connection.IsConnected()) {
    connection.SetTcpNoDelay(true);
    connection.Send(master_->GetMessage());
    master_->OnConnecting();
  } else {
    master_->OnDisconnecting(connection);
  }
}

void Session::OnMessageCallback(taotu::Connecting& connection,
                                taotu::IoBuffer* io_buffer, taotu::TimePoint) {
  ++messages_read_;
  bytes_read_ += io_buffer->GetReadableBytes();
  connection.Send(io_buffer);
}
