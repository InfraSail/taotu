/**
 * @file echo.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "EchoServer" which is an simple echo server.
 * @date 2022-02-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_EXAMPLE_SIMPLE_ECHO_H_
#define TAOTU_EXAMPLE_SIMPLE_ECHO_H_

#include <memory>

#include "../../src/server.h"

class EchoServer : taotu::NonCopyableMovable {
 public:
  typedef std::vector<taotu::EventManager*> EventManagers;

  EchoServer(const taotu::NetAddress& listen_address, bool should_reuse_port,
             size_t io_thread_amount = 3);
  ~EchoServer();

  // Start the server
  void Start();

 private:
  // Called after messages arriving
  void OnMessageCallback(taotu::Connecting& connection,
                         taotu::IoBuffer* io_buffer,
                         taotu::TimePoint time_point);

  EventManagers event_managers_;
  std::unique_ptr<taotu::Server> server_;
};

#endif  // !TAOTU_EXAMPLE_SIMPLE_ECHO_H_
