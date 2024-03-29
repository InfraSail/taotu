/**
 * @file http_server.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "HttpServer" which is a HTTP server.
 * @date 2022-04-27
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "http_server.h"

#include <stddef.h>

#include "http_parser.h"
#include "http_response.h"

HttpServer::HttpServer(const taotu::NetAddress& listen_address,
                       bool should_reuse_port, size_t io_thread_amount)
    : event_managers_(io_thread_amount, new taotu::EventManager),
      server_(std::make_unique<taotu::Server>(&event_managers_, listen_address,
                                              should_reuse_port)) {
  server_->SetConnectionCallback([this](taotu::Connecting& connection) {
    this->OnConnectionCallback(connection);
  });
  server_->SetMessageCallback([this](taotu::Connecting& connection,
                                     taotu::IoBuffer* io_buffer,
                                     taotu::TimePoint time_point) {
    this->OnMessageCallback(connection, io_buffer, std::move(time_point));
  });
}

HttpServer::~HttpServer() {
  size_t event_managers_size = event_managers_.size();
  for (size_t i = 0; i < event_managers_size; ++i) {
    delete event_managers_[i];
  }
  taotu::END_LOG();
}

void HttpServer::Start() { server_->Start(); }

void HttpServer::OnConnectionCallback(taotu::Connecting& connection) {
  if (connection.IsConnected()) {
    connection.SetContext<HttpParser>(llhttp_type_t::HTTP_REQUEST);
  }
}

void HttpServer::OnMessageCallback(taotu::Connecting& connection,
                                   taotu::IoBuffer* io_buffer,
                                   taotu::TimePoint time_point) {
  auto& parser_any = connection.GetMutableContext();
  auto& parser = std::any_cast<HttpParser&>(parser_any);
  std::string message{io_buffer->RetrieveAllAsString()};
  if (!parser.Parse(message.c_str(), message.size())) {
    connection.Send("HTTP/1.1 400 Bad Request\r\n\r\n");
    connection.ShutDownWrite();
  }
  OnRequest(connection, parser);
  parser.Reset();
}

void HttpServer::OnRequest(taotu::Connecting& connection,
                           const HttpParser& http_parser) {
  auto connection_info_optional = http_parser.GetHeader("Connection");
  auto connection_info = connection_info_optional.has_value()
                             ? connection_info_optional.value()
                             : std::string{};
  auto version_pair = http_parser.GetVersionPair();
  bool should_close = ("close" == connection_info ||
                       (1 == version_pair.first && 0 == version_pair.second &&
                        connection_info != "keep-alive"));
  HttpResponse http_response(should_close);
  HttpCallback_(http_parser, &http_response);
  taotu::IoBuffer io_buffer;
  http_response.AppendToIoBuffer(&io_buffer);
  connection.Send(&io_buffer);
  if (http_response.ShouldClose()) {
    connection.ShutDownWrite();
  }
}
