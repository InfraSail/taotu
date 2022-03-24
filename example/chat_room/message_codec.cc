/**
 * @file message_codec.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include "message_codec.h"

#include <stddef.h>
#include <stdint.h>

#include "../../src/logger.h"

enum {
  kHeadLength = sizeof(int32_t),
};

Codec::Codec(const MessageCallback& cb) : MessageCallback_(cb) {}

void Codec::OnMessage(taotu::Connecting& connection, taotu::IoBuffer* io_buffer,
                      taotu::TimePoint time_point) {
  while (io_buffer->GetReadableBytes() >= kHeadLength) {
    auto msg_len = io_buffer->GetReadableInt32();
    if (msg_len > 65536 || msg_len < 0) {  // Handle the error
      taotu::LOG(taotu::logger::kError, "Invalid length!!!");
      connection.ShutDownWrite();
      break;
    } else if (static_cast<int32_t>(io_buffer->GetReadableBytes()) >=
               msg_len + kHeadLength) {  // Only read the complete message
      io_buffer->Refresh(kHeadLength);
      std::string message(io_buffer->GetReadablePosition(), msg_len);
      MessageCallback_(connection, message, time_point);
      io_buffer->Refresh(static_cast<size_t>(msg_len));
    } else {  // Or jump out and wait for the complete message
      break;
    }
  }
}

void Codec::Send(taotu::Connecting* connection, const std::string& message) {
  taotu::IoBuffer io_buffer;
  io_buffer.Append(message.data(), message.size());  // Fill the message body
  auto msg_len = static_cast<int32_t>(message.size());
  io_buffer.SetHeadContentInt32(msg_len);
  connection->Send(&io_buffer);  // Fill the message head
}
