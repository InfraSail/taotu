/**
 * @file server.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-01-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_SERVER_H_
#define TAOTU_TAOTU_SERVER_H_

#include <atomic>
#include <memory>

#include "connecting.h"
#include "io_buffer.h"
#include "net_address.h"
#include "non_copyable_movable.h"
#include "time_point.h"

namespace taotu {

class ReactorManager;

/**
 * @brief  // TODO:
 *
 */
class Server : NonCopyableMovable {
 public:
  Server(const NetAddress& listen_address, int io_thread_amount = 6,
         bool should_reuse_port = false);
  ~Server();

  void SetConnectionCallback(const std::function<void(Connecting&)>& cb) {}
  void SetMessageCallback(
      const std::function<void(Connecting&, IoBuffer*, TimePoint)>& cb) {}
  void SetWriteCompleteCallback(const std::function<void(Connecting&)>& cb) {}

  void Start();

 private:
  typedef std::unique_ptr<ReactorManager> ReactorManagerPtr;

  void DefaultOnConnectionCallback(Connecting& connection);
  void DefaultOnMessageCallback(Connecting& connection, IoBuffer* io_buffer,
                                TimePoint time_point);

  ReactorManagerPtr reactor_manager_;

  // TODO: the thread pool

  std::atomic_bool is_started_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_SERVER_H_
