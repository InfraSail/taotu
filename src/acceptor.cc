/**
 * @file acceptor.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Implementation of class "Acceptor" which is the acceptor of new TCP
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
#include <unistd.h>

#include <string>

#include "logger.h"
#include "net_address.h"
#include "poller.h"

using namespace taotu;

enum {
  kMaxEventAmount = 600000,
};

Acceptor::Acceptor(Poller* poller, const NetAddress& listen_address,
                   bool should_reuse_port)
    : accept_socketer_(::socket(listen_address.GetFamily(),
                                SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                                IPPROTO_TCP)),
      accept_eventer_(poller, accept_socketer_.Fd()),
      is_listening_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  if (accept_socketer_.Fd() < 0) {
    LOG_ERROR("Fail to initialize for acceptor!!!");
    ::exit(-1);
  }
#ifndef __linux__
  int flags = ::fcntl(accept_socketer_.Fd(), F_GETFL, 0);
  flags |= O_NONBLOCK;
  ::fcntl(accept_socketer_.Fd(), F_SETFL, flags);
  flags = ::fcntl(accept_socketer_.Fd(), F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ::fcntl(accept_socketer_.Fd(), F_SETFD, flags);
#endif
  accept_socketer_.SetReuseAddress(true);
  accept_socketer_.SetReusePort(should_reuse_port);
  accept_socketer_.BindAddress(listen_address);
  accept_eventer_.RegisterReadCallback([this](const TimePoint& time_point) {
    this->DoReading();
  });  // Register the accepting action as a reading event handler
}
Acceptor::~Acceptor() {
  LOG_DEBUG("Acceptor with fd(%d) is closing.", accept_socketer_.Fd());
  is_listening_ = false;
  ::close(idle_fd_);
}

void Acceptor::Listen() {
  is_listening_ = true;
  accept_socketer_.Listen();
  accept_eventer_.EnableReadEvents();
  LOG_DEBUG("Acceptor with fd(%d) is listening.", accept_socketer_.Fd());
}

void Acceptor::DoReading() {
  NetAddress peer_address;
  int conn_fd = accept_socketer_.Accept(&peer_address);
  if (conn_fd >= 0 && conn_fd <= kMaxEventAmount) {
    if (NewConnectionCallback_) {
      NewConnectionCallback_(conn_fd, peer_address);
    } else {
      LOG_ERROR("Acceptor with fd(%d) is closing!!!", accept_socketer_.Fd());
      ::close(conn_fd);
    }
  } else {
    LOG_ERROR("Acceptor with Fd(%d) failed to accept a new TCP connection!!!",
              accept_socketer_.Fd());
    if (errno == EMFILE) {  // If it fails to connect, clear the waiting list of
                            // listening
      ::close(idle_fd_);
      idle_fd_ = ::accept(accept_socketer_.Fd(), NULL, NULL);
      ::close(idle_fd_);
      idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}
