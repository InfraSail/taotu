/**
 * @file chat_server.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "ChatServer" which is a chat server.
 * @date 2022-03-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_EXAMPLE_CHAT_ROOM_CHAT_SERVER_H_
#define TAOTU_EXAMPLE_CHAT_ROOM_CHAT_SERVER_H_

#include <memory>
#include <set>
#include <string>

#include "../../src/server.h"
#include "message_codec.h"

class ChatServer : taotu::NonCopyableMovable {
 public:
  typedef std::vector<taotu::EventManager*> EventManagers;

  ChatServer(const taotu::NetAddress& listen_address, bool should_reuse_port,
             size_t io_thread_amount = 3, size_t calculation_thread_amount = 0);
  ~ChatServer();

  // Start the server
  void Start();

 private:
  typedef std::set<taotu::Connecting*> ConnectionSet;

  // Called after one connection creating and before one connection destroying
  void OnConnectionCallback(taotu::Connecting& connection);

  // Called after messages arriving
  void OnCodecMessage(taotu::Connecting& connection, const std::string& message,
                      taotu::TimePoint);

  EventManagers event_managers_;
  std::unique_ptr<taotu::Server> server_;
  Codec codec_;
  ConnectionSet connections_;
  taotu::MutexLock connections_lock_;
};

#endif  // !TAOTU_EXAMPLE_CHAT_ROOM_CHAT_SERVER_H_
