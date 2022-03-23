/**
 * @file chat_client.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-03-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_EXAMPLE_CHAT_ROOM_CHAT_CLIENT_H_
#define TAOTU_EXAMPLE_CHAT_ROOM_CHAT_CLIENT_H_

#include <memory>
#include <set>
#include <string>

#include "../../src/client.h"
#include "message_codec.h"

class ChatClient : taotu::NonCopyableMovable {
 public:
  ChatClient(const taotu::NetAddress& connect_address);

  void Connect();
  void Disconnect();

  void Write(const std::string& message);

 private:
  void OnConnection(taotu::Connecting& connection);
  void OnCodecMessage(taotu::Connecting&, const std::string& message,
                      taotu::TimePoint);

  std::shared_ptr<taotu::EventManager> event_manager_;
  std::unique_ptr<taotu::Client> client_;
  Codec codec_;
  taotu::Connecting* connection_;
  taotu::MutexLock connection_lock_;
};

#endif  // !TAOTU_EXAMPLE_CHAT_ROOM_CHAT_CLIENT_H_
