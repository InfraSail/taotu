/**
 * @file acceptor.h
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Declaration of class "Acceptor" which is the acceptor of new TCP
 * connection requests from clients and create the connections.
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#ifndef TAOTU_SRC_ACCEPTOR_H_
#define TAOTU_SRC_ACCEPTOR_H_

#include <functional>

#include "event_manager.h"
#include "eventer.h"
#include "net_address.h"
#include "non_copyable_movable.h"
#include "socketer.h"

namespace taotu {

class Poller;

/**
 * @brief "Acceptor" is dedicated to handle new TCP connection requests and
 * build the connections in the main thread (then register it into corresponding
 * I/O threads).
 *
 */
class Acceptor : NonCopyableMovable {
 public:
  typedef std::function<void(int, const NetAddress&)> NewConnectionCallback;

  Acceptor(Poller* poller, const NetAddress& listen_address,
           bool should_reuse_port);
  ~Acceptor();

  // Get the file descriptor of this accepting socket
  int Fd() const { return accept_socketer_.Fd(); }

  // Listen to the port of the IP address
  void Listen();

  bool IsListening() const { return is_listening_; }

  void RegisterNewConnectionCallback(const NewConnectionCallback& cb) {
    NewConnectionCallback_ = cb;
  }

  // Accept a TCP connection request, create this connection, allocate a file
  // descriptor of this connecting socket and record its net address info
  void DoReading();

 private:
  // Socketer which is about configurations of the socket
  Socketer accept_socketer_;

  // Eventer which is about event's setting
  Eventer accept_eventer_;

  bool is_listening_;

  // For discarding failed connections
  int idle_fd_;

  // Be called when a new TCP connection request wait for being handled
  NewConnectionCallback NewConnectionCallback_;
};

}  // namespace taotu

#endif  // !TAOTU_SRC_ACCEPTOR_H_
