/**
 * @file connector.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief  // TODO:
 * @date 2021-12-12
 *
 * @copyright Copyright (c) 2021 Sigma711
 *
 */

#include "connector.h"

#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "logger.h"

using namespace taotu;

Connector::Connector(Poller* poller, const NetAddress& server_address)
    : server_address_(server_address), state_(kConnecting) {
  int socket_fd =
      ::socket(server_address_.GetFamily(),
               SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (socket_fd < 0) {
    LOG(logger::kError, "Fail to initialize for connector!!!");
    ::exit(-1);
  }
  eventer_ = std::make_unique<Eventer>(poller, socket_fd);
}
Connector::~Connector() {}

void Connector::Connect() {
  // TODO:
  int socket_fd = eventer_->Fd();
  int status = ::connect(socket_fd, server_address_.GetNetAddress(),
                         static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  int saved_errno = (0 == status) ? 0 : errno;
  switch (saved_errno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      // connecting(socket_fd);
      break;
    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      // retry(socket_fd);
      break;
    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      // LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
      // ::close(saved_errno);
      break;
    default:
      // LOG_SYSERR << "Unexpected error in Connector::startInLoop " <<
      // savedErrno;
      // ::close(saved_errno);
      //// connectErrorCallback_();
      break;
  }
}
