/**
 * @file http_response.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "HttpResponse" which is a HTTP response
 * message.
 * @date 2022-04-29
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "http_response.h"

#include <stdio.h>
#include <string.h>

HttpResponse::HttpResponse(bool should_close)
    : major_version_(1),
      minor_version_(1),
      status_(200),
      status_message_("OK"),
      should_close_(should_close),
      include_body_(true),
      has_content_length_override_(false),
      content_length_override_(0) {}

void HttpResponse::SetVersion(uint8_t major_version, uint8_t minor_version) {
  major_version_ = major_version;
  minor_version_ = minor_version;
}

void HttpResponse::SetStatus(uint16_t status,
                             const std::string& status_message) {
  status_ = status;
  status_message_ = status_message;
}

void HttpResponse::SetContentType(const std::string& content_type) {
  header_fields_["Content-Type"] = content_type;
}

void HttpResponse::SetBody(const std::string& body) {
  body_ = body;
  include_body_ = true;
  has_content_length_override_ = false;
}

void HttpResponse::SetBodyLength(size_t length) {
  content_length_override_ = length;
  has_content_length_override_ = true;
}

void HttpResponse::SetIncludeBody(bool include) { include_body_ = include; }

void HttpResponse::AddHeaderField(const std::string& field_name,
                                  const std::string& field_content) {
  header_fields_[field_name] = field_content;
}

void HttpResponse::AppendToIoBuffer(taotu::IoBuffer* io_buffer) {
  char buf[32];
  ::snprintf(buf, sizeof(buf), "HTTP/%u.%u %u ", major_version_, minor_version_,
             status_);
  io_buffer->Append(buf, ::strlen(buf));
  io_buffer->Append(status_message_.c_str(), status_message_.size());
  io_buffer->Append("\r\n", 2);

  if (should_close_) {
    io_buffer->Append("Connection: close\r\n", 19);
  } else {
    size_t content_length =
        has_content_length_override_ ? content_length_override_ : body_.size();
    ::snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", content_length);
    io_buffer->Append(buf, ::strlen(buf));
    io_buffer->Append("Connection: keep-alive\r\n", 24);
  }

  for (const auto& header_field : header_fields_) {
    io_buffer->Append(header_field.first.c_str(), header_field.first.size());
    io_buffer->Append(": ", 2);
    io_buffer->Append(header_field.second.c_str(), header_field.second.size());
    io_buffer->Append("\r\n", 2);
  }

  io_buffer->Append("\r\n", 2);
  if (include_body_) {
    io_buffer->Append(body_.c_str(), body_.size());
  }
}
