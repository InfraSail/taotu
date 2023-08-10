/**
 * @file pingpong_server.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "PingpongServer" which is a pingpong server.
 * @date 2022-03-28
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_EXAMPLE_PINGPONG_PINGPONG_CLIENT_H_
#define TAOTU_EXAMPLE_PINGPONG_PINGPONG_CLIENT_H_

#include <memory>

#include "../../src/server.h"

class PingpongServer : taotu::NonCopyableMovable {
 public:
  typedef std::vector<taotu::EventManager*> EventManagers;

  PingpongServer(const taotu::NetAddress& listen_address,
                 bool should_reuse_port, size_t io_thread_amount = 5,
                 size_t calculation_thread_amount = 0);
  ~PingpongServer();

  // Start the server
  void Start();

 private:
  // Called after one connection creating and before one connection destroying
  void OnConnectionCallback(taotu::Connecting& connection);

  // Called after messages arriving
  void OnMessageCallback(taotu::Connecting& connection,
                         taotu::IoBuffer* io_buffer,
                         taotu::TimePoint time_point);

  EventManagers event_managers_;
  std::unique_ptr<taotu::Server> server_;
};

#endif  // !TAOTU_EXAMPLE_PINGPONG_PINGPONG_CLIENT_H_
