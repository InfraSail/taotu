/**
 * @file main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Main entrance of the HTTP server.
 * @date 2022-04-27
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "http_server.h"

void OnRequest(const HttpParser& http_parser, HttpResponse* http_response) {
  time_t time_now;
  ::time(&time_now);
  struct tm* time_tm = ::gmtime(&time_now);
  char buf[100];
  ::strftime(buf, 100, "%a, %d %b %Y %T %Z", time_tm);
  http_response->AddHeaderField("Date", buf);
  http_response->AddHeaderField("Transfer-Encoding", "chunked");
  http_response->AddHeaderField("Server", "taotu-http-server");
  if (http_parser.GetMethod() == "GET") {
    if (http_parser.GetUrl() == "/") {
      http_response->SetStatus(200, "OK");
      http_response->SetContentType("text/html");
      http_response->SetBody(
          "<html><head><title>taotu</title></head><body><h1>Hello</h1>Welcome "
          "to taotu!</body></html>");
    } else if (http_parser.GetUrl() == "/hello") {
      http_response->SetStatus(200, "OK");
      http_response->SetContentType("text/html");
      http_response->SetBody(
          "<html><head><title>taotu</title></head><body><h1>Hello</h1>Hello, "
          "taotu!</body></html>");
    }
  }
}

// Call it by:
// './http_server [port [amount-of-I/O-threads
// [amount-of-calculation-threads]]]'
int main(int argc, char* argv[]) {
  if (1 == argc) {
    HttpServer http_server{taotu::NetAddress{4567}, false};
    http_server.SetHttpCallback(
        [](const HttpParser& http_parser, HttpResponse* http_response) {
          OnRequest(http_parser, http_response);
        });
    http_server.Start();
  } else if (2 == argc) {
    HttpServer http_server{taotu::NetAddress{static_cast<uint16_t>(
                               std::stoi(std::string{argv[1]}))},
                           false};
    http_server.SetHttpCallback(
        [](const HttpParser& http_parser, HttpResponse* http_response) {
          OnRequest(http_parser, http_response);
        });
    http_server.Start();
  } else if (3 == argc) {
    HttpServer http_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]}))};
    http_server.SetHttpCallback(
        [](const HttpParser& http_parser, HttpResponse* http_response) {
          OnRequest(http_parser, http_response);
        });
    http_server.Start();
  } else {
    HttpServer http_server{
        taotu::NetAddress{
            static_cast<uint16_t>(std::stoi(std::string{argv[1]}))},
        false, static_cast<size_t>(std::stoi(std::string{argv[2]})),
        static_cast<size_t>(std::stoi(std::string{argv[3]}))};
    http_server.SetHttpCallback(
        [](const HttpParser& http_parser, HttpResponse* http_response) {
          OnRequest(http_parser, http_response);
        });
    http_server.Start();
  }
  return 0;
}
