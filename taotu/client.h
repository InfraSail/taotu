/**
 * @file client.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2022-01-22
 *
 * @copyright Copyright (c) 2022 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_CLIENT_H_
#define TAOTU_TAOTU_CLIENT_H_

#include <memory>
#include <utility>

#include "non_copyable_movable.h"
#include "reactor_manager.h"

namespace taotu {

/**
 * @brief  // TODO:
 *
 */
class Client : NonCopyableMovable {
 public:
  Client(const NetAddress& server_address, bool should_retry_);
  ~Client();

  void SetConnectionCallback(std::function<void(Connecting&)> cb) {
    reactor_manager_->SetConnectionCallback(std::move(cb));
  }
  void SetMessageCallback(
      std::function<void(Connecting&, IoBuffer*, TimePoint)> cb) {
    reactor_manager_->SetMessageCallback(std::move(cb));
  }
  void SetWriteCompleteCallback(std::function<void(Connecting&)> cb) {
    reactor_manager_->SetWriteCompleteCallback(std::move(cb));
  }

  void Connect();
  void Stop();

 private:
  typedef std::unique_ptr<ClientReactorManager> ClientReactorManagerPtr;

  ClientReactorManagerPtr reactor_manager_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_CLIENT_H_
