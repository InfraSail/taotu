/**
 * @file acceptor.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_ACCEPTOR_H_
#define TAOTU_TAOTU_ACCEPTOR_H_

#include <functional>

#include "event_manager.h"
#include "eventer.h"
#include "non_copyable_movable.h"
#include "reactor.h"
#include "socketer.h"

namespace taotu {

/**
 * @brief  // TODO:
 *
 */
class Acceptor : NonCopyableMovable {
 public:
  typedef std::function<void(int, const SocketAddress&)> NewConnectionCallback;

  Acceptor(const SocketAddress& listen_fd, bool reuse_port);
  ~Acceptor();

  void Listen();
  bool IsListening() const { return is_listening_; }

  int Accept();

 private:
  Socketer accept_soketer_;
  bool is_listening_;
  int idle_fd_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_ACCEPTOR_H_
