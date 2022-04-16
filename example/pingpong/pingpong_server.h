/**
 * @file pingpong_server.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
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
  PingpongServer(const taotu::NetAddress& listen_address,
                 bool should_reuse_port, size_t io_thread_amount = 5,
                 size_t calculation_thread_amount = 0);
  ~PingpongServer() { delete event_manager_; }

  void Start();

 private:
  void OnConnectionCallback(taotu::Connecting& connection);
  void OnMessageCallback(taotu::Connecting& connection,
                         taotu::IoBuffer* io_buffer,
                         taotu::TimePoint time_point);

  taotu::EventManager* event_manager_;
  std::unique_ptr<taotu::Server> server_;
};

#endif  // !TAOTU_EXAMPLE_PINGPONG_PINGPONG_CLIENT_H_
