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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <utility>

#include "logger.h"

using namespace taotu;

Acceptor::Acceptor(const NetAddress& listen_address, bool reuse_port)
    : accept_soketer_(::socket(listen_address.GetFamily(),
                               SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                               IPPROTO_TCP)),
      is_listening_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  accept_soketer_.SetReuseAddress(true);
  accept_soketer_.SetReusePort(reuse_port);
  accept_soketer_.BindAddress(listen_address);
}
Acceptor::~Acceptor() {
  is_listening_ = false;
  ::close(idle_fd_);
}

void Acceptor::Listen() {
  is_listening_ = true;
  accept_soketer_.Listen();
}

int Acceptor::Accept() {
  SocketAddress peer_address;
  int conn_fd = accept_soketer_.Accept(&peer_address);
  if (conn_fd >= 0) {
    return conn_fd;
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
  return -1;
}
