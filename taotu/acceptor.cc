/**
 * @file acceptor.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-03
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "acceptor.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

#include <string>
#include <utility>

#include "logger.h"

using namespace taotu;

Acceptor::Acceptor(Eventer* eventer, const struct sockaddr_in6* listen_fd,
                   bool reuse_port)
    : eventer_(eventer),
      accept_soketer_(::socket(listen_fd->sin6_family,
                               SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                               IPPROTO_TCP)),
      accept_filer_(eventer, accept_soketer_.Fd()),
      is_listening_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  accept_soketer_.SetReuseAddress(true);
  accept_soketer_.SetReusePort(reuse_port);
  accept_soketer_.BindAddress(
      reinterpret_cast<const struct sockaddr*>(listen_fd));
  accept_filer_.RegisterReadCallBack(std::bind(&Acceptor::Accept, this));
}
Acceptor::~Acceptor() {
  is_listening_ = false;
  accept_filer_.DisableAllEvents();
  accept_filer_.RemoveMyself();
  ::close(accept_filer_.Fd());
  ::close(idle_fd_);
}

void Acceptor::RegisterNewConnectionCallback(NewConnectionCallback cb) {
  NewConnectionCallback_ = std::move(cb);
}

void Acceptor::Listen() {
  is_listening_ = true;
  accept_soketer_.Listen();
  accept_filer_.DisableReadEvents();
}

bool Acceptor::IsListening() { return is_listening_; }

void Acceptor::Accept() {
  struct sockaddr_in6 addr;
  int conn_fd = accept_soketer_.Accept(&addr);
  if (conn_fd >= 0) {
    if (NewConnectionCallback_) {
      NewConnectionCallback_(conn_fd, &addr);
    } else {
      ::close(conn_fd);
    }
  } else {
    LOG(logger::kError, "Fd(" + std::to_string(accept_soketer_.Fd()) +
                            ") could not accept a new TCP connection");
    if (errno == EMFILE) {
      ::close(idle_fd_);
      idle_fd_ = ::accept(accept_soketer_.Fd(), NULL, NULL);
      ::close(idle_fd_);
      idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}
