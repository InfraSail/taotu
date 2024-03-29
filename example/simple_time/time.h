/**
 * @file time.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "TimeServer" which is a simple time server.
 * @date 2022-03-03
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_EXAMPLE_SIMPLE_TIME_H_
#define TAOTU_EXAMPLE_SIMPLE_TIME_H_

#include "../../src/server.h"

class TimeServer : taotu::NonCopyableMovable {
 public:
  typedef std::vector<taotu::EventManager*> EventManagers;

  TimeServer(const taotu::NetAddress& listen_address, bool should_reuse_port,
             size_t io_thread_amount = 3);
  ~TimeServer();

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

#endif  // !TAOTU_EXAMPLE_SIMPLE_TIME_H_
