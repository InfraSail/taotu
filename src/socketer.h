/**
 * @file socketer.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "Socketer" which is the encapsulation of socket
 * file descriptor.
 * @date 2021-12-08
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_SOCKETER_H_
#define TAOTU_SRC_SOCKETER_H_

#include <sys/socket.h>
#ifndef __linux__
#include <fcntl.h>
#endif

#include "net_address.h"
#include "non_copyable_movable.h"

#ifndef __linux__
#define SOCK_NONBLOCK 0
#define SOCK_CLOEXEC 0
#endif

namespace taotu {

/**
 * @brief "Socketer" provides the ability of creating new connections and
 * setting a few of its socket options.
 *
 */
class Socketer : NonCopyableMovable {
 public:
  explicit Socketer(int socket_fd);
  ~Socketer();

  // Get the file descriptor of this socket
  int Fd() const { return socket_fd_; }

  // Bind local net address info(IP address, port and so on) (For "Acceptor")
  void BindAddress(const NetAddress& local_address) const;

  // Listen to the port of the IP address (For "Acceptor")
  void Listen() const;

  // Accept a connection request, create this connection, allocate a file
  // descriptor of this connecting socket and record its net address info (For
  // "Acceptor")
  int Accept(NetAddress* peer_address) const;

  // Shut down writing-end(self)
  void ShutdownWrite() const;

  void SetTcpNoDelay(bool on) const;
  void SetReuseAddress(bool on) const;
  void SetReusePort(bool on) const;
  void SetKeepAlive(bool on) const;

 private:
  void Close() const;

  // The file descriptor of this socket
  mutable int socket_fd_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_SOCKETER_H_
