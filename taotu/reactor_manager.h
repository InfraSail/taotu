/**
 * @file reactor_manager.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-16
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_REACTOR_MANAGER_H_
#define TAOTU_TAOTU_REACTOR_MANAGER_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "acceptor.h"
#include "connecting.h"
#include "net_address.h"
#include "non_copyable_movable.h"

namespace taotu {

class Balancer;

/**
 * @brief  // TODO:
 *
 */
class ServerReactorManager : NonCopyableMovable {
 public:
  typedef Connecting::NormalCallback NormalCallback;
  typedef Connecting::OnMessageCallback MessageCallback;

  typedef std::vector<std::unique_ptr<EventManager>> EventManagers;

  ServerReactorManager(const NetAddress& listen_address,
                       int io_thread_amount = 6,
                       bool should_reuse_port = false);
  ~ServerReactorManager();

  void SetConnectionCallback(const NormalCallback& cb) {
    ConnectionCallback_ = cb;
  }
  void SetMessageCallback(const MessageCallback& cb) { MessageCallback_ = cb; }
  void SetWriteCompleteCallback(const NormalCallback& cb) {
    WriteCompleteCallback_ = cb;
  }
  void SetCloseCallback(const NormalCallback& cb) { CloseCallback_ = cb; }

  void Loop();

 private:
  typedef std::unique_ptr<Acceptor> AcceptorPtr;
  typedef std::unique_ptr<Balancer> BalancerPtr;

  AcceptorPtr acceptor_;
  EventManagers event_managers_;
  BalancerPtr balancer_;

  NormalCallback ConnectionCallback_;
  MessageCallback MessageCallback_;
  NormalCallback WriteCompleteCallback_;
  NormalCallback CloseCallback_;

  bool should_stop_;
};

/**
 * @brief  // TODO:
 *
 */
class ClientReactorManager : NonCopyableMovable {};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_REACTOR_MANAGER_H_
