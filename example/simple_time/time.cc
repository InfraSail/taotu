/**
 * @file time.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "TimeServer" which is a simple time server.
 * @date 2022-03-03
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "time.h"

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>

TimeServer::TimeServer(const taotu::NetAddress& listen_address,
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
TimeServer::~TimeServer() {
  size_t event_managers_size = event_managers_.size();
  for (size_t i = 0; i < event_managers_size; ++i) {
    delete event_managers_[i];
  }
  taotu::END_LOG();
}

void TimeServer::Start() { server_->Start(); }

void TimeServer::OnMessageCallback(taotu::Connecting& connection,
                                   taotu::IoBuffer* io_buffer,
                                   taotu::TimePoint time_point) {
  std::string message{io_buffer->RetrieveAllAsString()};
  ssize_t msg_len = message.size();
  message = message.substr(0, msg_len - 1);
  taotu::LOG_DEBUG("Fd(%d) is receiving %u bytes(%s) at %lld.", connection.Fd(),
                   message.size(),
                   message.substr(0, message.size() - 1).c_str(),
                   time_point.GetMicroseconds());
  int64_t now_time = time_point.GetMicroseconds();
  time_t seconds = static_cast<time_t>(now_time / (1000 * 1000));
  struct tm tm_time;
  ::gmtime_r(&seconds, &tm_time);
  char buf[64] = {0};
  snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
           static_cast<int>(now_time % (1000 * 1000)));
  std::string data(buf);
  data += '\n';
  connection.Send(data);
}
