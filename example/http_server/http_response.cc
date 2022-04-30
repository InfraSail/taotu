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
      status_(1000),
      should_close_(should_close) {}

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
    ::snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", body_.size());
    io_buffer->Append(buf, sizeof(buf));
    io_buffer->Append("Connection: Keep-Alive\r\n", 24);
  }

  for (const auto& header_field : header_fields_) {
    io_buffer->Append(header_field.first.c_str(), sizeof(header_field.first));
    io_buffer->Append(": ", 2);
    io_buffer->Append(header_field.second.c_str(), sizeof(header_field.second));
    io_buffer->Append("\r\n", 2);
  }

  io_buffer->Append("\r\n", 2);
  io_buffer->Append(body_.c_str(), body_.size());
}
