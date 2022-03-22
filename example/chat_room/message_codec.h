/**
 * @file message_codec.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-21
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_EXAMPLE_CHAT_ROOM_MESSAGE_CODEC_H_
#define TAOTU_EXAMPLE_CHAT_ROOM_MESSAGE_CODEC_H_

#include <endian.h>

#include <functional>
#include <string>

#include "../../src/connecting.h"
#include "../../src/io_buffer.h"
#include "../../src/non_copyable_movable.h"

class Codec : taotu::NonCopyableMovable {
 public:
  typedef std::function<void(taotu::Connecting& connection,
                             const std::string& message, taotu::TimePoint)>
      MessageCallback;

  explicit Codec(const MessageCallback& cb);

  void OnMessage(taotu::Connecting& connection, taotu::IoBuffer* io_buffer,
                 taotu::TimePoint time_point);

  void Send(taotu::Connecting* connection, const std::string& message);

 private:
  MessageCallback MessageCallback_;
};

#endif  // !TAOTU_EXAMPLE_CHAT_ROOM_MESSAGE_CODEC_H_
