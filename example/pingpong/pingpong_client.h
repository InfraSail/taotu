/**
 * @file pingpong_client.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
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

/**
 * @brief  // TODO:
 *
 */
class PingpongClient : taotu::NonCopyableMovable {
 public:
  PingpongClient(const taotu::NetAddress& server_address, int block_size,
                 int session_count, int timeout, int thread_count);
  ~PingpongClient();

  void Start();

  const std::string& GetMessage() const { return message_; }

  void OnConnecting();
  void OnDisconnecting(taotu::Connecting& connection);

 private:
  typedef std::vector<taotu::EventManager*> EventManagers;

  void DoWithTimeout();

  EventManagers event_managers_;
  int session_count_;
  int timeout_;
  std::vector<std::unique_ptr<Session>> sessions_;
  std::string message_;
  std::atomic_int32_t conn_num_;
  std::unique_ptr<taotu::Balancer> balancer_;
};

/**
 * @brief  // TODO:
 *
 */
class Session : taotu::NonCopyableMovable {
 public:
  Session(taotu::EventManager* event_manager,
          const taotu::NetAddress& server_address, PingpongClient* client);

  void Start();
  void Stop();

  int64_t GetBytesRead() const { return bytes_read_; }
  int64_t GetMessagesRead() const { return messages_read_; }

 private:
  void OnConnectionCallback(taotu::Connecting& connection);
  void OnMessageCallback(taotu::Connecting& connection,
                         taotu::IoBuffer* io_buffer, taotu::TimePoint);

  taotu::Client client_;
  PingpongClient* master_;
  int64_t bytes_read_;
  int64_t messages_read_;
};

#endif  // !TAOTU_EXAMPLE_PINGPONG_PINGPONG_CLIENT_H_
