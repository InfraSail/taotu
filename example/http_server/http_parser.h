/**
 * @file http_parser.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "HttpParser" which is a HTTP parser based on
 * "llhttp".
 * @date 2022-04-28
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_EXAMPLE_HTTP_SERVER_HTTP_PARSER_H_
#define TAOTU_EXAMPLE_HTTP_SERVER_HTTP_PARSER_H_

#include <llhttp.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class HttpParser {
 public:
  explicit HttpParser(llhttp_type_t type);

  int OnMessageBegin(llhttp_t* parser);
  int OnStatus(llhttp_t* parser, const char* str, size_t str_length);
  int OnUrl(llhttp_t* parser, const char* str, size_t str_length);
  int OnHeaderField(llhttp_t* parser, const char* str, size_t str_length);
  int OnHeaderValue(llhttp_t* parser, const char* str, size_t str_length);
  int OnHeadersComplete(llhttp_t* parser);
  int OnBody(llhttp_t* parser, const char* str, size_t str_length);
  int OnMessageComplete(llhttp_t* parser);

  bool Parse(const char* data, size_t data_size);

  std::pair<uint8_t, uint8_t> GetVersionPair() const;

  bool ShouldUpgrade() const;

  bool ShouldKeepAlive() const;

  std::string GetMethod() const;
  std::string GetUrl() const;

  std::optional<std::string> GetHeader(const std::string& key) const;

  std::string GetBody() const;

  void Reset();

 private:
  static llhttp_settings_t parsing_settings_;

  uint8_t major_version_;
  uint8_t minor_version_;
  uint8_t upgrade_;
  uint8_t keep_alive_;

  time_t start_parsing_time_;
  time_t header_end_time_;
  time_t message_end_time_;

  std::string method_;
  std::string url_;
  std::string status_;

  struct pair {
    std::string key;
    std::string value;
  };
  std::vector<pair> key_value_pairs_;

  std::string body_;

  llhttp_t parser_;

  std::unordered_map<std::string, std::string> header_fields_;
};

#endif  // !TAOTU_EXAMPLE_HTTP_SERVER_HTTP_PARSER_H_
