/**
 * @file pingpong_client.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "PingpongClient" which is a pingpong total client
 * and class "Session" which manage one connection to the server.
 * @date 2022-03-28
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_EXAMPLE_PINGPONG_PINGPONG_CLIENT_H_
#define TAOTU_EXAMPLE_PINGPONG_PINGPONG_CLIENT_H_

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "../../src/client.h"

class Session;

class PingpongClient : taotu::NonCopyableMovable {
 public:
  PingpongClient(const taotu::NetAddress& server_address, size_t block_size,
                 size_t session_count, int timeout, size_t thread_count);
  ~PingpongClient();

  // Start the client
  void Start();

  // Get the string for the I/O test
  const std::string& GetMessage() const { return message_; }

  // Called after one connection creating
  void OnConnecting();

  // Called before one connection destroying
  void OnDisconnecting(taotu::Connecting& connection);

 private:
  typedef std::vector<taotu::EventManager*> EventManagers;

  void DoWithTimeout();

  EventManagers event_managers_;
  size_t session_count_;
  int timeout_;
  std::vector<std::unique_ptr<Session>> sessions_;
  std::string message_;
  std::atomic_size_t conn_num_;
  std::unique_ptr<taotu::Balancer> balancer_;
};

class Session : taotu::NonCopyableMovable {
 public:
  Session(taotu::EventManager* event_manager,
          const taotu::NetAddress& server_address,
          PingpongClient* master_client);

  // Start the session
  void Start();

  // Stop the session
  void Stop();

  int64_t GetBytesRead() const { return bytes_read_; }
  int64_t GetMessagesRead() const { return messages_read_; }

 private:
  // Called after the connection creating and before the connection destroying
  void OnConnectionCallback(taotu::Connecting& connection);

  // Called after messages arriving
  void OnMessageCallback(taotu::Connecting& connection,
                         taotu::IoBuffer* io_buffer, taotu::TimePoint);

  taotu::Client client_;
  PingpongClient* master_client_;
  int64_t bytes_read_;
  int64_t messages_read_;
};

#endif  // !TAOTU_EXAMPLE_PINGPONG_PINGPONG_CLIENT_H_
