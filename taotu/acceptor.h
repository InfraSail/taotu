/**
 * @file acceptor.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include <functional>

#include "eventer.h"
#include "filer.h"
#include "non_copyable_movable.h"
#include "socketer.h"

#ifndef TAOTU_TAOTU_ACCEPTOR_H_
#define TAOTU_TAOTU_ACCEPTOR_H_

namespace taotu {

/**
 * @brief  // TODO:
 *
 */
class Acceptor : NonCopyableMovable {
 public:
  typedef std::function<void(int, const SocketAddress&)> NewConnectionCallback;

  Acceptor(Eventer* eventer, const SocketAddress& listen_fd, bool reuse_port);
  ~Acceptor();

  void RegisterNewConnectionCallback(NewConnectionCallback cb);

  void Listen();
  bool IsListening();

 private:
  void Accept();

  const Eventer* eventer_;
  Socketer accept_soketer_;
  Filer accept_filer_;
  bool is_listening_;
  NewConnectionCallback NewConnectionCallback_;
  int idle_fd_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_ACCEPTOR_H_
