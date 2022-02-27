/**
 * @file echo.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-02-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#include <memory>

#include "../../../taotu/server.h"

class EchoServer : taotu::NonCopyableMovable {
 public:
  EchoServer(const taotu::NetAddress& listen_address, bool should_reuse_port);

  void Start();

 private:
  void OnConnectionCallback(taotu::Connecting& connection);
  void OnMessageCallback(taotu::Connecting& connection,
                         taotu::IoBuffer* io_buffer,
                         taotu::TimePoint time_point);

  std::unique_ptr<taotu::Server> server_;
};
