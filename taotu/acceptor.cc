/**
 * @file acceptor.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Acceptor" which is the accetor of new
 * connection requests from clients and create the connections.
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "acceptor.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <utility>

#include "eventer.h"
#include "logger.h"
#include "net_address.h"
#include "poller.h"

using namespace taotu;

Acceptor::Acceptor(Poller* poller, const NetAddress& listen_address,
                   bool should_reuse_port)
    : accept_soketer_(::socket(listen_address.GetFamily(),
                               SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                               IPPROTO_TCP)),
      accept_eventer_(poller, accept_soketer_.Fd()),
      is_listening_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  if (accept_soketer_.Fd() < 0) {
    LOG(logger::kError, "Fail to initialize for acceptor!!!");
    ::exit(-1);
  }
  accept_soketer_.SetReuseAddress(true);
  accept_soketer_.SetReusePort(should_reuse_port);
  accept_soketer_.BindAddress(listen_address);
  accept_eventer_.RegisterReadCallback(std::bind(&Acceptor::DoReading, this));
}
Acceptor::~Acceptor() {
  is_listening_ = false;
  ::close(idle_fd_);
}

void Acceptor::Listen() {
  is_listening_ = true;
  accept_soketer_.Listen();
  accept_eventer_.EnableReadEvents();
}

// TODO: Discard
int Acceptor::Accept(NetAddress* peer_address) {
  int conn_fd = accept_soketer_.Accept(peer_address);
  if (conn_fd >= 0) {
    return conn_fd;
  } else {
    LOG(logger::kError, "Fd(" + std::to_string(accept_soketer_.Fd()) +
                            ") could not accept a new TCP connection");
    if (errno == EMFILE) {  // If it fails to connect, clear the waiting list of
                            // listening
      ::close(idle_fd_);
      idle_fd_ = ::accept(accept_soketer_.Fd(), NULL, NULL);
      ::close(idle_fd_);
      idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
  return -1;
}

void Acceptor::DoReading() {
  NetAddress peer_address;
  int conn_fd = accept_soketer_.Accept(&peer_address);
  if (conn_fd >= 0) {
    if (NewConnectionCallback_) {
      NewConnectionCallback_(conn_fd, peer_address);
    } else {
      LOG(logger::kError, "Acceptor with fd(" +
                              std::to_string(accept_soketer_.Fd()) +
                              ") is closing!!!");
      ::close(conn_fd);
    }
  } else {
    LOG(logger::kError, "Acceptor with Fd(" +
                            std::to_string(accept_soketer_.Fd()) +
                            ") could not accept a new TCP connection!!!");
    if (errno == EMFILE) {  // If it fails to connect, clear the waiting list of
                            // listening
      ::close(idle_fd_);
      idle_fd_ = ::accept(accept_soketer_.Fd(), NULL, NULL);
      ::close(idle_fd_);
      idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}
