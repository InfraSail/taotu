/**
 * @file socketer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  //TODO:
 * @date 2021-12-08
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_TAOTU_SOCKETER_H_
#define TAOTU_TAOTU_SOCKETER_H_

#include <netinet/in.h>
#include <sys/socket.h>

#include "non_copyable_movable.h"

namespace taotu {

/**
 * @brief  // TODO:
 *
 */
class Socketer : NonCopyableMovable {
 public:
  Socketer(int socket_fd);
  ~Socketer();

  int Fd() const;

  void BindAddress(const struct sockaddr* local_address);
  void Listen();
  int Accept(struct sockaddr_in6* peer_address);

  void ShutdownWrite();

  void SetTcpNoDelay(bool on);
  void SetReuseAddress(bool on);
  void SetReusePort(bool on);
  void SetKeepAlive(bool on);

 private:
  const int socket_fd_;
};

}  // namespace taotu

#endif  // !TAOTU_TAOTU_SOCKETER_H_
