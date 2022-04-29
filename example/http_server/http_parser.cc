/**
 * @file http_parser.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "HttpParser" which is a HTTP parser based on
 * "llhttp".
 * @date 2022-04-28
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "http_parser.h"

#include <stdio.h>
#include <time.h>

HttpParser::HttpParser(llhttp_type_t type) {
  llhttp_init(&parser_, type, &HttpParser::parsing_settings_);
  parser_.data = this;
}

int HttpParser::OnMessageBegin(llhttp_t* parser) {
  major_version_ = 0;
  minor_version_ = 0;
  upgrade_ = 0;
  keep_alive_ = 0;
  start_parsing_time_ = 0;
  header_end_time_ = 0;
  message_end_time_ = 0;
  method_.clear();
  url_.clear();
  status_.clear();
  key_value_pairs_.clear();
  body_.clear();
  start_parsing_time_ = ::time(NULL);
  return 0;
}
int HttpParser::OnStatus(llhttp_t* parser, const char* str, size_t str_length) {
  status_.append(str, str_length);
  return 0;
}
int HttpParser::OnUrl(llhttp_t* parser, const char* str, size_t str_length) {
  url_.append(str, str_length);
  return 0;
}
int HttpParser::OnHeaderField(llhttp_t* parser, const char* str,
                              size_t str_length) {
  key_value_pairs_.emplace_back();
  key_value_pairs_.back().key = std::string(str, str_length);
  return 0;
}
int HttpParser::OnHeaderValue(llhttp_t* parser, const char* str,
                              size_t str_length) {
  key_value_pairs_.back().value = std::string(str, str_length);
  return 0;
}
int HttpParser::OnHeadersComplete(llhttp_t* parser) {
  header_end_time_ = ::time(NULL);
  method_ = std::string(
      llhttp_method_name(static_cast<llhttp_method_t>(parser->method)));
  major_version_ = parser->http_major;
  minor_version_ = parser->http_minor;
  upgrade_ = parser->upgrade;
  keep_alive_ = llhttp_should_keep_alive(parser);
  return 0;
}
int HttpParser::OnBody(llhttp_t* parser, const char* str, size_t str_length) {
  body_.append(str, str_length);
  return 0;
}
int HttpParser::OnMessageComplete(llhttp_t* parser) {
  message_end_time_ = ::time(NULL);
  return 0;
}

bool HttpParser::Parse(const char* data, size_t data_size) {
  enum llhttp_errno error = llhttp_execute(&parser_, data, data_size);
  if (error != HPE_OK) {
    ::fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(error),
              parser_.reason);
    return false;
  }
  for (const auto& key_value : key_value_pairs_) {
    header_fields_[key_value.key] = key_value.value;
  }
  return true;
}

std::pair<uint8_t, uint8_t> HttpParser::GetVersionPair() const {
  return std::pair<uint8_t, uint8_t>{major_version_, minor_version_};
}

bool HttpParser::ShouldUpgrade() const { return upgrade_ == 1; }

bool HttpParser::ShouldKeepAlive() const { return keep_alive_ == 1; }

std::string HttpParser::GetMethod() const { return method_; }
std::string HttpParser::GetUrl() const { return url_; }

std::optional<std::string> HttpParser::GetHeader(const std::string& key) const {
  if (header_fields_.count(key) == 0) {
    return std::nullopt;
  }
  return std::optional<std::string>{header_fields_.at(key)};
}

std::string HttpParser::GetBody() const { return body_; }

void HttpParser::Reset() {
  major_version_ = 0;
  minor_version_ = 0;
  upgrade_ = 0;
  keep_alive_ = 0;
  start_parsing_time_ = 0;
  header_end_time_ = 0;
  message_end_time_ = 0;
  method_.clear();
  url_.clear();
  status_.clear();
  key_value_pairs_.clear();
  body_.clear();
  start_parsing_time_ = ::time(NULL);
}

llhttp_settings_t HttpParser::parsing_settings_{
    [](llhttp_t* parser) -> int {
      return reinterpret_cast<HttpParser*>(parser->data)
          ->OnMessageBegin(parser);
    },
    [](llhttp_t* parser, const char* str, size_t str_length) -> int {
      return reinterpret_cast<HttpParser*>(parser->data)
          ->OnUrl(parser, str, str_length);
    },
    [](llhttp_t* parser, const char* str, size_t str_length) -> int {
      return reinterpret_cast<HttpParser*>(parser->data)
          ->OnStatus(parser, str, str_length);
    },
    [](llhttp_t* parser, const char* str, size_t str_length) -> int {
      return reinterpret_cast<HttpParser*>(parser->data)
          ->OnHeaderField(parser, str, str_length);
    },
    [](llhttp_t* parser, const char* str, size_t str_length) -> int {
      return reinterpret_cast<HttpParser*>(parser->data)
          ->OnHeaderValue(parser, str, str_length);
    },
    [](llhttp_t* parser) -> int {
      return reinterpret_cast<HttpParser*>(parser->data)
          ->OnHeadersComplete(parser);
    },
    [](llhttp_t* parser, const char* str, size_t str_length) -> int {
      return reinterpret_cast<HttpParser*>(parser->data)
          ->OnBody(parser, str, str_length);
    },
    [](llhttp_t* parser) -> int {
      return reinterpret_cast<HttpParser*>(parser->data)
          ->OnMessageComplete(parser);
    }};
