/**
 * @file http_response.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "HttpResponse" which is a HTTP response message.
 * @date 2022-04-29
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_EXAMPLE_HTTP_SERVER_HTTP_RESPONSE_H_
#define TAOTU_EXAMPLE_HTTP_SERVER_HTTP_RESPONSE_H_

#include <map>
#include <string>

#include "../../src/io_buffer.h"

class HttpResponse {
 public:
  explicit HttpResponse(bool should_close);

  void SetVersion(uint8_t major_version, uint8_t minor_version);

  void SetStatus(uint16_t status, const std::string& status_message);

  void SetContentType(const std::string& content_type);

  void SetBody(const std::string& body);
  void SetBodyLength(size_t length);
  void SetIncludeBody(bool include);

  void AddHeaderField(const std::string& field_name,
                      const std::string& field_content);

  void SetClose(bool on) { should_close_ = on; }
  bool ShouldClose() const { return should_close_; }

  void AppendToIoBuffer(taotu::IoBuffer* io_buffer);

 private:
  uint8_t major_version_;
  uint8_t minor_version_;

  uint16_t status_;
  std::string status_message_;

  std::map<std::string, std::string> header_fields_;

  bool should_close_;
  std::string body_;
  bool include_body_;
  bool has_content_length_override_;
  size_t content_length_override_;
};

#endif  // !TAOTU_EXAMPLE_HTTP_SERVER_HTTP_RESPONSE_H_
