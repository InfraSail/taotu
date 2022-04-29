/**
 * @file http_server.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "HttpServer" which is a HTTP server.
 * @date 2022-04-27
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_EXAMPLE_HTTP_SERVER_HTTP_SERVER_H_
#define TAOTU_EXAMPLE_HTTP_SERVER_HTTP_SERVER_H_

#include <functional>
#include <memory>

#include "../../src/server.h"
#include "http_parser.h"
#include "http_response.h"

class HttpServer : taotu::NonCopyableMovable {
 public:
  HttpServer(const taotu::NetAddress& listen_address, bool should_reuse_port,
             size_t io_thread_amount = 4, size_t calculation_thread_amount = 0);
  ~HttpServer();

  // Start the server
  void Start();

  void SetHttpCallback(const std::function<void(const HttpParser&,
                                                HttpResponse*)>& HttpCallback) {
    HttpCallback_ = HttpCallback;
  }

 private:
  // Called after one connection creating and before one connection destroying
  void OnConnectionCallback(taotu::Connecting& connection);

  // Called after messages arriving
  void OnMessageCallback(taotu::Connecting& connection,
                         taotu::IoBuffer* io_buffer,
                         taotu::TimePoint time_point);

  // Called when a request arriving
  void OnRequest(taotu::Connecting& connection, const HttpParser& http_parser);

  taotu::EventManager* event_manager_;
  std::unique_ptr<taotu::Server> server_;

  std::function<void(const HttpParser&, HttpResponse*)> HttpCallback_;
};

#endif  // !TAOTU_EXAMPLE_HTTP_SERVER_HTTP_SERVER_H_
