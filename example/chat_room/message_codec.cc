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

#include <stdint.h>

#include "../../src/logger.h"

enum {
  kHeadLength = sizeof(int32_t),
};

Codec::Codec(const MessageCallback& cb) : MessageCallback_(cb) {}

void Codec::OnMessage(taotu::Connecting& connection, taotu::IoBuffer* io_buffer,
                      taotu::TimePoint time_point) {
  while (io_buffer->GetReadableBytes() >= kHeadLength) {
    int32_t msg_len = io_buffer->RetrieveInt<int32_t>();
    if (msg_len > 65536 || msg_len < 0) {
      taotu::LOG(taotu::logger::kError, "Invalid length!!!");
      connection.ShutDownWrite();
      break;
    } else if (io_buffer->GetReadableBytes() >= msg_len + kHeadLength) {
      io_buffer->Refresh(kHeadLength);
    } else {
      break;
    }
  }
}

void Codec::Send(taotu::Connecting* connection, const std::string& message) {}
