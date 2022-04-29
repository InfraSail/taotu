/**
 * @file chat_client.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "ChatClient" which is a chat client.
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
  ~ChatClient();

  // Start connecting
  void Connect();

  // Disconnect the connection
  void Disconnect();

  // Write the message with the specific header to the server
  void Write(const std::string& message);

 private:
  // Called after one connection creating and before one connection destroying
  void OnConnectionCallback(taotu::Connecting& connection);

  // Called after messages arriving
  void OnCodecMessage(taotu::Connecting&, const std::string& message,
                      taotu::TimePoint);

  std::shared_ptr<taotu::EventManager> event_manager_;
  std::unique_ptr<taotu::Client> client_;
  Codec codec_;
  taotu::Connecting* connection_;
  taotu::MutexLock connection_lock_;
};

#endif  // !TAOTU_EXAMPLE_CHAT_ROOM_CHAT_CLIENT_H_
